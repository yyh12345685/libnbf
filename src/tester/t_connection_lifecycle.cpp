#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "net/async_client_connect.h"
#include "net/sync_client_connect.h"
#include "protocol/http/http_protocol.h"
#include "protocol/rapid/rapid_protocol.h"
#include "service/service_manager.h"
#include "service/service_handle.h"
#include "message.h"

using namespace bdf;
struct Counts {
  std::atomic<int> live{0}, replies{0}, destroyed{0};
};
// Messages are freed with MessageFactory::Destroy, which (with MEM_PROFILE)
// expects MessageFactory::Allocate's header: never create them with new.
struct CountedRapid : RapidMessage {
  Counts& counts;
  explicit CountedRapid(Counts* c) : counts(*c) { ++counts.live; }
  ~CountedRapid() { --counts.live; }
};
struct CountedHttp : HeartBeatMessage {
  Counts& counts;
  explicit CountedHttp(Counts* c) : counts(*c) { ++counts.live; }
  ~CountedHttp() { --counts.live; }
};
struct RapidHeartbeats : RapidProtocol {
  Counts& counts;
  bool hold;
  RapidHeartbeats(Counts* c, bool pending) : counts(*c), hold(pending) {}
  EventMessage* HeartBeatRequest() override {
    auto* msg = MessageFactory::Allocate<CountedRapid>(&counts);
    msg->type_id = MessageType::kHeartBeatMessage;
    msg->direction = MessageBase::kOutgoingRequest;
    msg->command = 255;
    return msg;
  }
  bool Encode(EventMessage* msg, Buffer* output) override {
    return hold || RapidProtocol::Encode(msg, output);
  }
};
struct HttpHeartbeats : HttpProtocol {
  Counts& counts;
  bool hold;
  HttpHeartbeats(Counts* c, bool pending) : counts(*c), hold(pending) {}
  EventMessage* HeartBeatRequest() override {
    auto* msg = MessageFactory::Allocate<CountedHttp>(&counts);
    msg->direction = MessageBase::kOutgoingRequest;
    msg->heart_type = MessageType::kHttpMessage;
    msg->load_avg = 0;
    return msg;
  }
  bool Encode(EventMessage* msg, Buffer* output) override {
    return hold || HttpProtocol::Encode(msg, output);
  }
};
struct RapidClient : AsyncClientConnect {
  Counts& counts;
  explicit RapidClient(Counts* c) : AsyncClientConnect(500, 5), counts(*c) {}
  ~RapidClient() { ++counts.destroyed; }
  void OnDecodeMessage(EventMessage* msg) override {
    AsyncClientConnect::OnDecodeMessage(msg);
    if (++counts.replies == 3) OnClose(); // Exercise reconnect on the owning I/O loop.
  }
};
struct HttpClient : SyncClientConnect {
  Counts& counts;
  explicit HttpClient(Counts* c) : SyncClientConnect(500, 5, false), counts(*c) {}
  ~HttpClient() { ++counts.destroyed; }
  void OnDecodeMessage(EventMessage* msg) override {
    SyncClientConnect::OnDecodeMessage(msg);
    if (++counts.replies == 3) OnClose();
  }
};
template<class Predicate> void Wait(Predicate done) {
  auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!done() && std::chrono::steady_clock::now() < until)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  assert(done());
}
int ReservePort(int& fd) {
  fd = socket(AF_INET, SOCK_STREAM, 0); assert(fd >= 0);
  sockaddr_in address = {}; address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  assert(bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
  socklen_t size = sizeof(address);
  assert(getsockname(fd, reinterpret_cast<sockaddr*>(&address), &size) == 0);
  return ntohs(address.sin_port);
}
int main() {
  int hf, rf;
  int hp = ReservePort(hf), rp = ReservePort(rf);
  ServiceConfig config; config.io_thread_count = 2; config.service_handle_thread_count = 0;
  config.monitor_file_name = "/tmp/libnbf-lifecycle-monitor.log";
  ServerConfig http, rapid;
  http.address = "http://127.0.0.1:" + std::to_string(hp);
  rapid.address = "rapid://127.0.0.1:" + std::to_string(rp);
  config.server_config = {http, rapid};
  close(hf); close(rf);
  auto& service = service::GetServiceManager();
  assert(service.Init(config) == 0);
  assert(service.Start(new ServiceHandler) == 0);
  for (bool pending : {false, true}) {
    for (bool is_http : {false, true}) {
      for (int iteration = 0; iteration < 10; ++iteration) {
        Counts counts;
        ClientConnect* client;
        if (is_http) {
          // Connections and protocols are freed with BDF_DELETE: use BDF_NEW.
          client = BDF_NEW(HttpClient, &counts);
          client->SetProtocol(BDF_NEW(HttpHeartbeats, &counts, pending));
        } else {
          client = BDF_NEW(RapidClient, &counts);
          client->SetProtocol(BDF_NEW(RapidHeartbeats, &counts, pending));
        }
        client->SetIp("127.0.0.1"); client->SetPort(is_http ? hp : rp);
        assert(client->TryConnect() == 0);
        if (pending) Wait([&] { return counts.live.load() > 0; });
        else Wait([&] { return counts.replies.load() >= 6; });
        client->Stop();
        Wait([&] { return counts.destroyed.load() == 1; });
        assert(counts.live == 0);
      }
    }
  }
  // Wait past request and reconnect deadlines: no callback may touch a deleted client.
  std::this_thread::sleep_for(std::chrono::milliseconds(650));
  service.Stop();
}

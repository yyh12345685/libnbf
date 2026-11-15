// End-to-end: redis replies must reach the request they belong to.
// A fake redis server answers in tiny chunks (so chunk boundaries split
// replies), while requests with several commands, single-command requests
// and heartbeat PINGs are pipelined over one SyncClientConnect.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "client/client.h"
#include "service/service_manager.h"
#include "service/service_handle.h"
#include "app/config_info.h"
#include "message.h"

using namespace bdf;

// ---- fake redis server ---------------------------------------------------
// Parses RESP command arrays from the stream (partial input allowed) and
// answers PING/SET/GET like redis; any other command gets an error reply.
class FakeRedis {
public:
  FakeRedis() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd_ >= 0);
    int on = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    socklen_t len = sizeof(addr);
    assert(getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    port_ = ntohs(addr.sin_port);
    assert(listen(listen_fd_, 4) == 0);
    thread_ = std::thread([this] { Serve(); });
  }
  ~FakeRedis() {
    stop_ = true;
    thread_.join();
    close(listen_fd_);
  }
  int port() const { return port_; }
  int pings() const { return pings_; }

private:
  // Returns false if `in` does not hold a whole command yet.
  static bool TakeCommand(std::string& in, std::vector<std::string>& args) {
    args.clear();
    size_t pos = 0;
    auto line = [&](std::string& out) {
      size_t crlf = in.find("\r\n", pos);
      if (crlf == std::string::npos) return false;
      out = in.substr(pos, crlf - pos);
      pos = crlf + 2;
      return true;
    };
    std::string head;
    if (!line(head)) return false;
    assert(!head.empty() && head[0] == '*');
    int n = std::stoi(head.substr(1));
    for (int i = 0; i < n; ++i) {
      std::string len_line;
      if (!line(len_line)) return false;
      assert(len_line[0] == '$');
      size_t len = std::stoul(len_line.substr(1));
      if (in.size() < pos + len + 2) return false;
      args.push_back(in.substr(pos, len));
      pos += len + 2;
    }
    in.erase(0, pos);
    return true;
  }

  std::string Answer(const std::vector<std::string>& args) {
    if (args[0] == "PING") { ++pings_; return "+PONG\r\n"; }
    if (args[0] == "SET" && args.size() == 3) { db_[args[1]] = args[2]; return "+OK\r\n"; }
    if (args[0] == "GET" && args.size() == 2) {
      auto it = db_.find(args[1]);
      if (it == db_.end()) return "$-1\r\n";
      return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
    }
    return "-ERR unknown command\r\n";
  }

  void Serve() {
    int fd = -1;
    std::string in, out;
    size_t chunk = 1;
    while (!stop_) {
      if (fd < 0) {
        pollfd p = {listen_fd_, POLLIN, 0};
        if (poll(&p, 1, 10) == 1) fd = accept(listen_fd_, nullptr, nullptr);
        continue;
      }
      pollfd p = {fd, POLLIN, 0};
      if (poll(&p, 1, out.empty() ? 10 : 0) == 1) {
        char buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) { close(fd); fd = -1; in.clear(); out.clear(); continue; }
        in.append(buf, n);
        std::vector<std::string> args;
        while (TakeCommand(in, args)) out += Answer(args);
      }
      if (!out.empty()) {
        // 1..7 byte chunks with a pause: reply boundaries fall mid-chunk
        // and replies arrive across many reads on the client.
        size_t n = std::min(out.size(), chunk);
        chunk = chunk % 7 + 1;
        ssize_t w = write(fd, out.data(), n);
        if (w > 0) out.erase(0, w);
        std::this_thread::sleep_for(std::chrono::microseconds(30));
      }
    }
    if (fd >= 0) close(fd);
  }

  int listen_fd_ = -1;
  int port_ = 0;
  std::atomic<bool> stop_{false};
  std::atomic<int> pings_{0};
  std::map<std::string, std::string> db_; // server thread only
  std::thread thread_;
};

// ---- test ----------------------------------------------------------------
struct Results {
  std::mutex lock;
  int done = 0;
  std::vector<std::string> errors;
  void Fail(const std::string& what) {
    std::lock_guard<std::mutex> guard(lock);
    errors.push_back(what);
  }
};

static std::string Join(const std::vector<std::string>& v) {
  std::string s;
  for (const auto& x : v) s += "[" + x + "]";
  return s;
}

template<class Predicate> static bool WaitFor(Predicate done, int seconds) {
  auto until = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
  while (!done() && std::chrono::steady_clock::now() < until)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  return done();
}

int main() {
  FakeRedis server;

  ServiceConfig config;
  config.io_thread_count = 1;
  config.service_handle_thread_count = 1;
  config.monitor_file_name = "/tmp/libnbf-redis-pipeline-monitor.log";
  // The acceptor needs one listen address; port 0 = any free port.
  ServerConfig unused;
  unused.address = "rapid://127.0.0.1:0";
  config.server_config = {unused};
  auto& service = service::GetServiceManager();
  assert(service.Init(config) == 0);
  assert(service.Start(new ServiceHandler) == 0);

  // heartbeat every 5ms: PINGs interleave with the pipelined requests
  Client client("redis://127.0.0.1:" + std::to_string(server.port()), 3000, 5, false);
  assert(client.Start() == 0);
  assert(WaitFor([&] { return client.GetClientStatus() == Client::kWorking; }, 3));

  const int kRounds = 300;
  Results results;
  for (int i = 0; i < kRounds; ++i) {
    const std::string key = "k" + std::to_string(i);
    const std::string value = "v" + std::to_string(i);

    // Request A: two commands (binary-safe cmds first, then bodys).
    RedisMessage* a = MessageFactory::Allocate<RedisMessage>();
    a->AddCmd({"SET", key, value});
    a->bodys.push_back("GET" + std::string(1, '\1') + key);
    assert(client.Invoke(a, [&results, value](EventMessage* m) {
      RedisMessage* r = dynamic_cast<RedisMessage*>(m);
      std::vector<std::string> want = {"OK", value};
      if (!r || m->status != MessageBase::kStatusOK || r->replies.size() != 2 || r->bodys != want)
        results.Fail("A " + value + " got " + (r ? Join(r->bodys) : "?"));
      { std::lock_guard<std::mutex> g(results.lock); ++results.done; }
      MessageFactory::Destroy(m);
    }, "redis_pipeline_a"));

    // Request B: one command, must see exactly its own reply.
    RedisMessage* b = MessageFactory::Allocate<RedisMessage>();
    b->bodys.push_back("GET" + std::string(1, '\1') + key);
    assert(client.Invoke(b, [&results, value](EventMessage* m) {
      RedisMessage* r = dynamic_cast<RedisMessage*>(m);
      if (!r || m->status != MessageBase::kStatusOK || r->replies.size() != 1
          || r->bodys != std::vector<std::string>{value})
        results.Fail("B " + value + " got " + (r ? Join(r->bodys) : "?"));
      { std::lock_guard<std::mutex> g(results.lock); ++results.done; }
      MessageFactory::Destroy(m);
    }, "redis_pipeline_b"));

    // Request C: three commands, the middle one a redis error.
    RedisMessage* c = MessageFactory::Allocate<RedisMessage>();
    c->AddCmd({"GET", "missing-" + key});
    c->AddCmd({"BOGUS"});
    c->AddCmd({"GET", key});
    assert(client.Invoke(c, [&results, value](EventMessage* m) {
      RedisMessage* r = dynamic_cast<RedisMessage*>(m);
      bool ok = r && m->status == MessageBase::kStatusOK && r->replies.size() == 3
        && r->replies[0].IsNil() && r->replies[1].IsError() && r->HasError()
        && r->replies[2].str == value;
      if (!ok) results.Fail("C " + value + " got " + (r ? Join(r->bodys) : "?"));
      { std::lock_guard<std::mutex> g(results.lock); ++results.done; }
      MessageFactory::Destroy(m);
    }, "redis_pipeline_c"));
  }

  bool finished = WaitFor([&] {
    std::lock_guard<std::mutex> g(results.lock);
    return results.done == kRounds * 3;
  }, 30);
  for (const auto& e : results.errors) std::cerr << "mismatch: " << e << std::endl;
  std::cerr << "callbacks:" << results.done << "/" << kRounds * 3
            << " errors:" << results.errors.size() << " pings:" << server.pings() << std::endl;
  assert(finished);
  assert(results.errors.empty());
  assert(server.pings() > 0); // heartbeats really interleaved

  client.Stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  service.Stop();
  std::cout << "t_redis_pipeline passed" << std::endl;
  return 0;
}

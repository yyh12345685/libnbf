#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>
#include <cctype>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "net/connect.h"
#include "net/socket.h"
#include "event/event_driver.h"
#include "event/timer/timer.h"
#include "protocol/http/http_protocol.h"
#include "protocol/rapid/rapid_protocol.h"
#include "message.h"
#include "common/buffer.h"
using namespace bdf;
struct Callback : OnTimerBase {
  Timer* timer; uint64_t self = 0, other = 0; int calls = 0;
  void OnTimer(void*) override {
    ++calls;
    if (self) timer->DelTimer(self);
    if (other) timer->DelTimer(other);
  }
};
struct BackpressureConnection : Connecting {
  EventDriver& driver;
  int enabled = 0, disabled = 0;
  bool closed = false;
  int writes = 0;
  explicit BackpressureConnection(EventDriver& loop) : driver(loop) {}
  void Destroy() override {}
  bool IsServer() override { return false; }
  void OnClose() override { closed = true; driver.Del(fd_); Clean(); }
  void OnDecodeMessage(EventMessage* msg) override { MessageFactory::Destroy(msg); }
  int SetWriteInterest(bool set) override {
    if (set) ++enabled; else ++disabled;
    return driver.Modw(fd_, set);
  }
  int WriteSocket(const char* data, int size) override {
    // WSL socket buffering varies. Force one short write then EAGAIN;
    // subsequent writes still go through the real TCP socket and event loop.
    if (++writes == 2) { errno = EAGAIN; return -1; }
    return Connecting::WriteSocket(data, writes == 1 ? 4096 : size);
  }
  void Queue(const std::string& data) { assert(outbuf_.Write(data.data(), data.size())); }
  size_t Pending() { return outbuf_.GetReadingSize(); }
};
void TestBackpressure() {
  EventDriver driver; assert(driver.Init() >= 0);
  int listener = Socket::Listen("127.0.0.1", 0);
  assert(listener >= 0);
  sockaddr_in address = {};
  socklen_t length = sizeof(address);
  assert(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0);
  int pair[2];
  pair[1] = socket(AF_INET, SOCK_STREAM, 0);
  assert(pair[1] >= 0);
  assert(connect(pair[1], reinterpret_cast<sockaddr*>(&address), length) == 0);
  pollfd ready = {listener, POLLIN, 0};
  assert(poll(&ready, 1, 2000) == 1);
  pair[0] = Socket::Accept(listener);
  assert(pair[0] >= 0 && Socket::SetNonBlock(pair[1]));
  close(listener);
  int capacity = 4096;
  assert(setsockopt(pair[0], SOL_SOCKET, SO_SNDBUF, &capacity, sizeof(capacity)) == 0);
  BackpressureConnection con(driver); con.SetFd(pair[0]);
  assert(driver.Add(pair[0], &con) == 0 && driver.Modr(pair[0], true) == 0);
  std::string data(256 * 1024, 'x'), received;
  con.Queue(data); con.OnWrite();
  assert(con.Pending() > 0 && con.enabled == 1 && !con.closed);
  for (int i = 0; i < 1000 && (con.Pending() || received.size() < data.size()); ++i) {
    char bytes[16384]; ssize_t size;
    while ((size = read(pair[1], bytes, sizeof(bytes))) > 0) received.append(bytes, size);
    driver.Poll(1);
  }
  assert(received == data && con.Pending() == 0 && con.disabled == 1 && !con.closed);
  con.Queue("second send"); con.OnWrite();
  // The peer socket is nonblocking; WSL loopback may deliver asynchronously.
  pollfd readable = {pair[1], POLLIN, 0};
  assert(poll(&readable, 1, 2000) == 1);
  char bytes[64]; assert(read(pair[1], bytes, sizeof(bytes)) == 11);
  assert(con.enabled == 1 && con.disabled == 1);
  driver.Del(pair[0]); close(pair[1]);
}
static std::string EncodeLower(HttpProtocol& http, HttpMessage& msg) {
  Buffer out; assert(http.Encode(&msg, &out));
  std::string text(out.GetReading(), out.GetReadingSize());
  for (auto& ch : text) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
  return text;
}
void TestHttpHeaders() {
  HttpProtocol http; Buffer input; bool failed = false;
  std::string request = "GET / HTTP/1.1\r\nHost: h\r\nCookie: a=1\r\nX-A: x\r\n"
    "Cookie: b=2\r\nX-A: y\r\n\r\n";
  input.Write(request.data(), request.size());
  auto msg = dynamic_cast<HttpMessage*>(http.Decode(input, failed));
  assert(msg && !failed);
  assert(msg->http_info.headers["Cookie"] == "a=1; b=2");
  assert(msg->http_info.headers["X-A"] == "x, y");
  MessageFactory::Destroy(msg);

  HttpMessage empty_ok; empty_ok.InitReply(nullptr, 200);
  empty_ok.direction = MessageBase::kOutgoingResponse;
  assert(EncodeLower(http, empty_ok).find("content-length: 0\r\n") != std::string::npos);
  HttpMessage no_content; no_content.InitReply(nullptr, 204);
  no_content.direction = MessageBase::kOutgoingResponse;
  assert(EncodeLower(http, no_content).find("content-length") == std::string::npos);
  HttpMessage chunked; chunked.InitReply(nullptr, 200);
  chunked.direction = MessageBase::kOutgoingResponse;
  chunked.http_info.headers["Transfer-Encoding"] = "chunked";
  assert(EncodeLower(http, chunked).find("content-length") == std::string::npos);
  HttpMessage get; get.InitRequest("GET", true);
  get.direction = MessageBase::kOutgoingRequest; get.http_info.url = "/";
  assert(EncodeLower(http, get).find("content-length") == std::string::npos);
}
int main() {
  TestBackpressure();
  TestHttpHeaders();
  Timer timer;
  Callback first, second; first.timer = second.timer = &timer;
  TimerData td = {}; td.time_proc = &first;
  first.self = timer.AddTimer(0, td);
  td.time_proc = &second; timer.AddTimer(0, td);
  timer.ProcessTimer();
  assert(first.calls == 1 && second.calls == 1);
  td.time_proc = &second;
  timer.AddTimer(0, td);
  timer.DelTimer(first.self); // Expired handles must not cancel reused storage.
  timer.ProcessTimer(); assert(second.calls == 2);
  td.time_proc = &first; first.self = timer.AddTimer(0, td);
  td.time_proc = &second; first.other = timer.AddTimer(0, td);
  timer.ProcessTimer(); assert(first.calls == 2 && second.calls == 2);
  HttpProtocol http; Buffer input; bool failed = false; EventMessage* result = nullptr;
  std::string request = "POST /fragment HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nX-Test: abcdef\r\n\r\nhello";
  for (size_t i = 0; i < request.size(); ++i) {
    input.Write(&request[i], 1); result = http.Decode(input, failed);
    assert(!failed);
    if (i + 1 < request.size()) assert(result == nullptr);
  }
  auto msg = dynamic_cast<HttpMessage*>(result); assert(msg);
  assert(msg->http_info.body == "hello"); assert(msg->http_info.headers["X-Test"] == "abcdef");
  MessageFactory::Destroy(msg);
  RapidProtocol rapid; RapidMessage original; original.body = "fragmented rapid body";
  Buffer encoded; assert(rapid.Encode(&original, &encoded));
  Buffer fragments;
  for (size_t i = 0; i < encoded.GetReadingSize(); ++i) {
    fragments.Write(encoded.GetReading() + i, 1);
    result = rapid.Decode(fragments, failed); assert(!failed);
    if (i + 1 < encoded.GetReadingSize()) assert(result == nullptr);
  }
  auto rm = dynamic_cast<RapidMessage*>(result); assert(rm && rm->body == original.body);
  MessageFactory::Destroy(rm);
  EventMessage* hb = rapid.HeartBeatRequest();
  Buffer heartbuf; assert(rapid.Encode(hb, &heartbuf));
  result = rapid.Decode(heartbuf, failed);
  assert(result && result->type_id == MessageType::kHeartBeatMessage);
  MessageFactory::Destroy(result); MessageFactory::Destroy(hb);
  RapidMessage business; business.command = 255;
  Buffer business_frame; assert(rapid.Encode(&business, &business_frame));
  result = rapid.Decode(business_frame, failed);
  assert(result && result->type_id == MessageType::kRapidMessage);
  MessageFactory::Destroy(result);
  hb = http.HeartBeatRequest(); Buffer httpheart;
  assert(http.Encode(hb, &httpheart)); MessageFactory::Destroy(hb);
  EventMessage initialized(0, 123); assert(initialized.ctx == nullptr && initialized.msg_coro == nullptr);
  std::cout << "timer cancellation, HTTP/Rapid fragmentation, message initialization passed\n";
}

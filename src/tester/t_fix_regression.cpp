// Regression tests for the protocol-level fixes: HTTP encode validation,
// case-insensitive headers, Transfer-Encoding smuggling, interim/HEAD/EOF
// responses, URL helpers, large Redis replies and reply descriptors.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>
#include "protocol/http/http_protocol.h"
#include "protocol/http/http_util.h"
#include "protocol/redis/redis_protocol.h"
#include "common/string_util.h"
#include "common/buffer.h"
#include "net/connect.h"
#include "message.h"

using namespace bdf;

static std::string Encoded(HttpProtocol& http, EventMessage* msg, bool* ok) {
  Buffer out;
  *ok = http.Encode(msg, &out);
  return std::string(out.GetReading(), out.GetReadingSize());
}

static EventMessage* Feed(HttpProtocol& http, const std::string& data, bool& failed) {
  Buffer in;
  in.Write(data.data(), data.size());
  return http.Decode(in, failed);
}

static void TestHeaderInjectionRefused() {
  HttpProtocol http;
  bool ok = false;
  HttpMessage bad_value; bad_value.InitReply(nullptr, 200);
  bad_value.direction = MessageBase::kOutgoingResponse;
  bad_value.http_info.headers["Location"] = "/x\r\nSet-Cookie: evil=1";
  assert(Encoded(http, &bad_value, &ok).empty() && !ok);

  HttpMessage bad_name; bad_name.InitReply(nullptr, 200);
  bad_name.direction = MessageBase::kOutgoingResponse;
  bad_name.http_info.headers["X Bad"] = "v";
  Encoded(http, &bad_name, &ok); assert(!ok);

  HttpMessage bad_url; bad_url.InitRequest("GET", true);
  bad_url.direction = MessageBase::kOutgoingRequest;
  bad_url.http_info.url = "/a HTTP/1.1\r\nX: y";
  Encoded(http, &bad_url, &ok); assert(!ok);

  HttpMessage good; good.InitReply(nullptr, 418);
  good.direction = MessageBase::kOutgoingResponse;
  std::string wire = Encoded(http, &good, &ok);
  assert(ok && wire.find("HTTP/1.1 418 Unknown\r\n") == 0);
}

static void TestCaseInsensitiveHeaders() {
  HttpProtocol http; bool failed = false;
  auto* msg = dynamic_cast<HttpMessage*>(Feed(http,
    "GET /api/v1/users HTTP/1.1\r\nHost: a\r\nhost: b\r\ncontent-type: text/plain\r\n"
    "cookie: uid=7\r\n\r\n", failed));
  assert(msg && !failed);
  std::string v;
  assert(msg->GetHeader("Content-Type", &v) && v == "text/plain");
  assert(msg->GetHeader("HOST", &v) && v == "a, b");   // joined, not two keys
  assert(msg->http_info.headers.size() == 3);
  HttpParsedInfo info;
  HttpUtil::BuildHttpRequest(msg, info);
  assert(info.cookies_str == "uid=7");                 // lowercase "cookie"
  assert(info.path == "/api/v1/users");                // not cut at 2nd '/'
  MessageFactory::Destroy(msg);
}

static void TestTransferEncodingSmuggling() {
  HttpProtocol http; bool failed = false;
  EventMessage* msg = Feed(http,
    "POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: gzip, chunked\r\n\r\n"
    "0\r\n\r\nGET /admin HTTP/1.1\r\n\r\n", failed);
  assert(failed);
  if (msg) MessageFactory::Destroy(msg);

  HttpProtocol ok_http; failed = false;
  msg = Feed(ok_http,
    "POST / HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: Chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n", failed);
  auto* hm = dynamic_cast<HttpMessage*>(msg);
  assert(hm && !failed && hm->http_info.body == "hello");
  MessageFactory::Destroy(msg);
}

// Client side: encode a request so the protocol tracks it, then feed replies.
static void SendRequest(HttpProtocol& http, const char* method) {
  HttpMessage req; req.InitRequest(method, true);
  req.direction = MessageBase::kOutgoingRequest;
  req.http_info.url = "/";
  bool ok = false;
  Encoded(http, &req, &ok);
  assert(ok);
}

static void TestInterimAndHeadResponses() {
  HttpProtocol http; bool failed = false;
  SendRequest(http, "HEAD");
  SendRequest(http, "GET");
  Buffer in;
  std::string replies =
    "HTTP/1.1 100 Continue\r\n\r\n"                          // interim: dropped
    "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n"            // HEAD: no body
    "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nhi";         // GET
  in.Write(replies.data(), replies.size());
  auto* head = dynamic_cast<HttpMessage*>(http.Decode(in, failed));
  assert(head && !failed && head->status_code == 200 && head->http_info.body.empty());
  auto* get = dynamic_cast<HttpMessage*>(http.Decode(in, failed));
  assert(get && !failed && get->http_info.body == "hi");
  assert(http.Decode(in, failed) == nullptr && !failed);
  MessageFactory::Destroy(head);
  MessageFactory::Destroy(get);
}

static void TestEofDelimitedResponse() {
  HttpProtocol http; bool failed = false;
  SendRequest(http, "GET");
  Buffer in;
  std::string reply = "HTTP/1.0 200 OK\r\n\r\nhello";
  in.Write(reply.data(), reply.size());
  assert(http.Decode(in, failed) == nullptr && !failed);   // body ends at EOF
  auto* msg = dynamic_cast<HttpMessage*>(http.OnEof());
  assert(msg && msg->http_info.body == "hello");
  MessageFactory::Destroy(msg);
}

static void TestUrlDecode() {
  assert(common::UrlDecode("a%") == "a%");
  assert(common::UrlDecode("a%4") == "a%4");
  assert(common::UrlDecode("%zz") == "%zz");
  assert(common::UrlDecode("a%20b+c") == "a b c");
}

static void TestLargeRedisReply() {
  RedisProtocol redis; bool failed = false;
  const size_t big = 9 * 1024 * 1024;                       // above the old 8MB cap
  std::string header = "$" + std::to_string(big) + "\r\n";
  Buffer in;
  in.Write(header.data(), header.size());
  std::string chunk(64 * 1024, 'x');
  size_t sent = 0;
  while (sent < big) {
    size_t n = std::min(chunk.size(), big - sent);
    in.Write(chunk.data(), n);
    sent += n;
    assert(redis.Decode(in, failed) == nullptr && !failed);
  }
  in.Write("\r\n", 2);
  auto* msg = dynamic_cast<RedisMessage*>(redis.Decode(in, failed));
  assert(msg && !failed && msg->bodys.size() == 1 && msg->bodys[0].size() == big);
  MessageFactory::Destroy(msg);
}

static void TestDescriptor() {
  int dummy = 0;
  Connecting* fake = reinterpret_cast<Connecting*>(&dummy);
  int64_t d = Connecting::MakeDescriptor(fake, 0x12345);
  assert(Connecting::DescriptorConnect(d) == fake);
  assert(Connecting::DescriptorGen(d) == 0x2345);
}

int main() {
  TestHeaderInjectionRefused();
  TestCaseInsensitiveHeaders();
  TestTransferEncodingSmuggling();
  TestInterimAndHeadResponses();
  TestEofDelimitedResponse();
  TestUrlDecode();
  TestLargeRedisReply();
  TestDescriptor();
  std::cout << "t_fix_regression passed" << std::endl;
  return 0;
}

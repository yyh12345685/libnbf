#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "protocol/redis/redis_protocol.h"
#include "protocol/redis/redis_parser.h"
#include "message.h"
#include "common/buffer.h"
using namespace bdf;

static const std::string S(1, RedisParser::kSep);

static RedisMessage* DecodeOne(RedisProtocol& redis, Buffer& input, bool& failed) {
  EventMessage* msg = redis.Decode(input, failed);
  if (!msg) return nullptr;
  assert(msg->type_id == MessageType::kRedisMessage);
  RedisMessage* rmsg = dynamic_cast<RedisMessage*>(msg);
  assert(rmsg);
  return rmsg;
}

// deliver the reply one byte at a time: nullptr and !failed until the last byte
static void ByteByByte(const std::string& reply, const std::vector<std::string>& bodys) {
  RedisProtocol redis;
  Buffer input;
  bool failed = false;
  for (size_t i = 0; i < reply.size(); ++i) {
    input.Write(&reply[i], 1);
    RedisMessage* msg = DecodeOne(redis, input, failed);
    assert(!failed);
    if (i + 1 < reply.size()) {
      assert(msg == nullptr);
      assert(input.GetReadingSize() == i + 1);  // nothing discarded
    } else {
      assert(msg != nullptr);
      assert(msg->bodys == bodys);
      assert(input.GetReadingSize() == 0);
      MessageFactory::Destroy(msg);
    }
  }
}

// whole reply in one buffer
static void Whole(const std::string& reply, const std::vector<std::string>& bodys) {
  RedisProtocol redis;
  Buffer input;
  bool failed = false;
  input.Write(reply.data(), reply.size());
  RedisMessage* msg = DecodeOne(redis, input, failed);
  assert(!failed);
  assert(msg != nullptr);
  if (msg->bodys != bodys) {
    std::cerr << "unexpected bodys for " << reply << std::endl;
    assert(false);
  }
  assert(input.GetReadingSize() == 0);
  MessageFactory::Destroy(msg);
  assert(redis.Decode(input, failed) == nullptr && !failed);
}

static void Fails(const std::string& data) {
  RedisProtocol redis;
  Buffer input;
  bool failed = false;
  input.Write(data.data(), data.size());
  EventMessage* msg = redis.Decode(input, failed);
  assert(failed);
  assert(msg == nullptr);
}

typedef std::vector<std::string> V;

// Decode one reply and return its single structured entry (caller frees msg).
static RedisMessage* Decoded(const std::string& reply) {
  static RedisProtocol redis;
  Buffer input; bool failed = false;
  input.Write(reply.data(), reply.size());
  RedisMessage* msg = DecodeOne(redis, input, failed);
  assert(msg && !failed && msg->replies.size() == 1);
  return msg;
}

int main() {
  // bodys hold raw values: no "1"/"0"/"-1" prefixes, no '\1' separators.
  ByteByByte("$5\r\nhello\r\n", V{"hello"});
  ByteByByte("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", V{"foo", "bar"});

  Whole("*0\r\n", V{});
  Whole("*-1\r\n", V{});
  Whole("$-1\r\n", V{});
  Whole("$0\r\n\r\n", V{""});
  Whole(":42\r\n", V{"42"});
  Whole("-ERR x\r\n", V{"ERR x"});
  Whole("+OK\r\n", V{"OK"});
  Whole("+PONG\r\n", V{"PONG"});  // not swallowed any more
  Whole("*2\r\n:1\r\n$1\r\na\r\n", V{"1", "a"});
  // nil element keeps its position as "" (MGET with a missing key)
  Whole("*3\r\n$-1\r\n+OK\r\n-ERR e\r\n", V{"", "OK", "ERR e"});
  Whole("*2\r\n*2\r\n:1\r\n:2\r\n$1\r\nz\r\n", V{"1", "2", "z"});
  Whole("$4\r\na\r\nb\r\n", V{"a\r\nb"});  // binary safe

  // structured view: type, error, nil, integer, nesting
  {
    RedisMessage* m = Decoded("-ERR wrong type\r\n");
    assert(m->replies[0].type == RedisReply::kError && m->replies[0].str == "ERR wrong type");
    assert(m->HasError());
    MessageFactory::Destroy(m);
    m = Decoded(":-7\r\n");
    assert(m->replies[0].type == RedisReply::kInteger && m->replies[0].integer == -7);
    assert(!m->HasError());
    MessageFactory::Destroy(m);
    m = Decoded("$-1\r\n");
    assert(m->replies[0].IsNil() && m->bodys.empty());
    MessageFactory::Destroy(m);
    m = Decoded("$0\r\n\r\n");  // empty string is not nil
    assert(m->replies[0].type == RedisReply::kString && m->replies[0].str.empty());
    MessageFactory::Destroy(m);
    m = Decoded("+OK\r\n");
    assert(m->replies[0].type == RedisReply::kStatus && m->replies[0].str == "OK");
    MessageFactory::Destroy(m);
    m = Decoded("*3\r\n$1\r\na\r\n$-1\r\n*1\r\n:5\r\n");
    const RedisReply& r = m->replies[0];
    assert(r.type == RedisReply::kArray && r.elements.size() == 3);
    assert(r.elements[0].str == "a" && r.elements[1].IsNil());
    assert(r.elements[2].type == RedisReply::kArray && r.elements[2].elements[0].integer == 5);
    MessageFactory::Destroy(m);
  }

  // pipelined replies: one message per Decode call
  {
    RedisProtocol redis;
    Buffer input;
    bool failed = false;
    std::string two = "+OK\r\n$3\r\nbar\r\n*1\r\n$1\r\n";  // third one incomplete
    input.Write(two.data(), two.size());
    RedisMessage* m1 = DecodeOne(redis, input, failed);
    assert(m1 && !failed && m1->bodys == V{"OK"});
    RedisMessage* m2 = DecodeOne(redis, input, failed);
    assert(m2 && !failed && m2->bodys == V{"bar"});
    assert(redis.Decode(input, failed) == nullptr && !failed);
    assert(input.GetReadingSize() == 8);
    input.Write("x\r\n", 3);
    RedisMessage* m3 = DecodeOne(redis, input, failed);
    assert(m3 && !failed && m3->bodys == V{"x"});
    assert(input.GetReadingSize() == 0);
    MessageFactory::Destroy(m1);
    MessageFactory::Destroy(m2);
    MessageFactory::Destroy(m3);
  }

  // real errors close the connection
  Fails("!garbage\r\n");
  Fails("x");
  Fails("$abc\r\n");
  Fails("$-2\r\n");
  Fails("*-5\r\n");
  Fails(":12a\r\n");
  Fails("$3\r\nfooXY");                // payload not followed by CRLF
  Fails("*2\r\n$1\r\na\r\n?\r\n");     // bad element type
  Fails("$12345678901234567890123456789012345");  // length line never ends

  // heartbeat
  {
    RedisProtocol redis;
    EventMessage* hb = redis.HeartBeatRequest();
    assert(hb && hb->type_id == MessageType::kHeartBeatMessage);
    Buffer out;
    assert(redis.Encode(hb, &out));
    assert(std::string(out.GetReading(), out.GetReadingSize()) == "*1\r\n$4\r\nPING\r\n");
    MessageFactory::Destroy(hb);
  }

  // normal request encoding is unchanged
  {
    RedisProtocol redis;
    RedisMessage* req = MessageFactory::Allocate<RedisMessage>();
    req->bodys.push_back("SET" + S + "k" + S + "v");
    Buffer out;
    assert(redis.Encode(req, &out));
    assert(std::string(out.GetReading(), out.GetReadingSize()) ==
      "*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$1\r\nv\r\n");
    MessageFactory::Destroy(req);
  }

  {
    // ParseCmd: consecutive separators give an empty argument, one trailing
    // separator is ignored, an empty command name is rejected.
    std::string out;
    assert(RedisParser::ParseCmd(std::string("GET\1k\1"), out));
    assert(out == "*2\r\n$3\r\nGET\r\n$1\r\nk\r\n");
    assert(RedisParser::ParseCmd(std::string("SET\1k\1\1"), out));
    assert(out == "*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$0\r\n\r\n");
    assert(RedisParser::ParseCmd(std::string("SET\1k\1\1v"), out));
    assert(out == "*4\r\n$3\r\nSET\r\n$1\r\nk\r\n$0\r\n\r\n$1\r\nv\r\n");
    assert(!RedisParser::ParseCmd(std::string("\1k"), out));
    assert(!RedisParser::ParseCmd(std::string(""), out));
    // cmds: binary-safe, including \1 and \0 inside a value, before bodys.
    RedisProtocol redis;
    RedisMessage* req = MessageFactory::Allocate<RedisMessage>();
    std::string value("a\1b", 3); value.push_back('\0');
    req->AddCmd({"SET", "k", value});
    req->bodys.push_back(std::string("GET\1k"));
    Buffer buf;
    assert(redis.Encode(req, &buf));
    std::string wire(buf.GetReading(), buf.GetReadingSize());
    std::string expect("*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$4\r\n");
    expect.append(value).append("\r\n*2\r\n$3\r\nGET\r\n$1\r\nk\r\n");
    assert(wire == expect);
    MessageFactory::Destroy(req);
    // The two replies of that 2-command request come back as ONE message,
    // even byte by byte; the next request's reply stays separate.
    std::string replies("+OK\r\n$1\r\nv\r\n:7\r\n");
    const size_t first_len = 12; // "+OK\r\n" + "$1\r\nv\r\n"
    RedisMessage* next = MessageFactory::Allocate<RedisMessage>();
    next->bodys.push_back(std::string("INCR\1n"));
    Buffer scratch; assert(redis.Encode(next, &scratch));
    MessageFactory::Destroy(next);
    Buffer in; bool failed = false; EventMessage* got = nullptr;
    for (size_t i = 0; i < first_len; ++i) {
      in.Write(&replies[i], 1);
      got = redis.Decode(in, failed);
      assert(!failed);
      if (i + 1 < first_len) assert(got == nullptr);
    }
    RedisMessage* first = dynamic_cast<RedisMessage*>(got);
    assert(first && first->replies.size() == 2 && first->bodys == (V{"OK", "v"}));
    MessageFactory::Destroy(first);
    in.Write(replies.data() + first_len, replies.size() - first_len);
    RedisMessage* second = dynamic_cast<RedisMessage*>(redis.Decode(in, failed));
    assert(second && second->replies.size() == 1 && second->replies[0].integer == 7);
    assert(in.GetReadingSize() == 0);
    MessageFactory::Destroy(second);
    // Reset (connection closed) forgets outstanding requests.
    redis.Reset();
  }
  std::cout << "t_redis_protocol passed" << std::endl;
  return 0;
}

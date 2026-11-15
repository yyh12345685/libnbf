#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace bdf {

// One decoded RESP reply.
struct RedisReply {
  enum Type {
    kString = 0,  // "$" bulk string
    kStatus,      // "+" simple string, e.g. OK / PONG
    kError,       // "-" error, str holds the message ("ERR ...")
    kInteger,     // ":" integer, in integer (str holds the decimal text)
    kNil,         // "$-1" / "*-1"
    kArray,       // "*" array, in elements
  };

  int type = kNil;
  std::string str;
  int64_t integer = 0;
  std::vector<RedisReply> elements;

  bool IsError() const { return type == kError; }
  bool IsNil() const { return type == kNil; }

  // Values in order, nested arrays flattened. A nil inside an array becomes
  // "" so positions stay aligned (e.g. MGET with missing keys); a nil reply
  // gives no value. Use type / IsNil() to tell nil from an empty string.
  void Flatten(std::vector<std::string>& out, bool top = true) const {
    switch (type) {
    case kNil:
      if (!top) out.push_back(std::string());
      break;
    case kArray:
      for (const auto& e : elements) e.Flatten(out, false);
      break;
    default:
      out.push_back(str);
      break;
    }
  }
};

}

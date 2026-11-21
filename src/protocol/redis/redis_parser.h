
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "protocol/redis/redis_reply.h"

namespace bdf {

class RedisParser {
public:
  static const char kSep = '\1';

  enum {
    kParseError = -1,
    kParseOk = 0,
    kParseNeedMore = 1,
  };

  // limits used to reject hostile / corrupt input instead of buffering forever
  // 64MB: a buggy or hostile server must not make every pooled connection
  // buffer half a gigabyte (the protocol cap is tied to this value).
  static const int64_t kMaxBulkLen = 64 * 1024 * 1024;
  static const int64_t kMaxArrayLen = 1024 * 1024;
  static const int kMaxNestDepth = 32;

public:
  // Parse exactly ONE complete RESP reply from data[0, len).
  // return kParseError(-1): malformed input, the stream can not be resynced;
  //        kParseOk(0): one reply parsed into *out, *consumed is its size;
  //        kParseNeedMore(1): reply is incomplete, nothing consumed.
  // need_hint (optional): on kParseNeedMore, a lower bound of the total
  // bytes the reply needs (known once a bulk length header is parsed), so
  // the caller can skip re-parsing until that much is buffered.
  static int ParseReply(
    const char* data, size_t len, size_t* consumed, RedisReply* out,
    size_t* need_hint = nullptr);

  // Encode one command given as kSep-separated arguments ("SET\1k\1v").
  // Consecutive kSep give an empty argument ("SET\1k\1\1" sets k to "");
  // one trailing kSep is ignored ("GET\1k\1" == "GET\1k"). An argument
  // containing kSep, or an empty last argument, needs EncodeArgs / cmds.
  static bool ParseCmd(const std::string& in, std::string& out);

  // Binary-safe: encode one command from its argument list.
  static bool EncodeArgs(const std::vector<std::string>& args, std::string& out);

private:
  // find "\r\n" in data[from, len), return its position or -1
  static int64_t FindCRLF(const char* data, size_t len, size_t from);
  // strict signed decimal parse of data[begin, end)
  static bool ParseInt(const char* data, size_t begin, size_t end, int64_t* val);
  // parse one element starting at data[*pos]; on kParseOk *pos is advanced.
  static int ParseElement(const char* data, size_t len, size_t* pos,
    int depth, RedisReply* out, size_t* need_hint);
};

}

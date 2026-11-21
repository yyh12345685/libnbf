
//  参考文档：http://doc.redisfans.com/topic/protocol.html
//  参考文档为官方文档，协议一节
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "redis_parser.h"

namespace bdf {

int64_t RedisParser::FindCRLF(const char* data, size_t len, size_t from) {
  if (len < 2) return -1;
  for (size_t i = from; i + 1 < len; ++i) {
    if (data[i] == '\r' && data[i + 1] == '\n') return (int64_t)i;
  }
  return -1;
}

bool RedisParser::ParseInt(const char* data, size_t begin, size_t end, int64_t* val) {
  if (begin >= end) return false;
  bool neg = false;
  size_t i = begin;
  if (data[i] == '-' || data[i] == '+') {
    neg = (data[i] == '-');
    ++i;
    if (i == end) return false;
  }
  uint64_t v = 0;
  for (; i < end; ++i) {
    if (data[i] < '0' || data[i] > '9') return false;
    v = v * 10 + (uint64_t)(data[i] - '0');
    if (v > (uint64_t)INT64_MAX) return false;
  }
  *val = neg ? -(int64_t)v : (int64_t)v;
  return true;
}

int RedisParser::ParseElement(const char* data, size_t len, size_t* pos,
  int depth, RedisReply* out, size_t* need_hint) {
  size_t start = *pos;
  if (start >= len) return kParseNeedMore;
  if (depth > kMaxNestDepth) return kParseError;

  const char type = data[start];
  if (type != '+' && type != '-' && type != ':' && type != '$' && type != '*') {
    return kParseError;
  }

  int64_t crlf = FindCRLF(data, len, start + 1);
  if (crlf < 0) {
    // a length / integer line can not be long: a missing CRLF after many
    // bytes means a corrupt stream, not a slow one
    static const size_t kMaxNumLine = 32;
    if (type != '+' && type != '-' && len - start > kMaxNumLine) return kParseError;
    return kParseNeedMore;
  }
  const size_t line_begin = start + 1;
  const size_t line_end = (size_t)crlf;
  const size_t after_line = line_end + 2;

  switch (type) {
  case '-':
  case '+':
    out->type = (type == '-') ? RedisReply::kError : RedisReply::kStatus;
    out->str.assign(data + line_begin, line_end - line_begin);
    *pos = after_line;
    return kParseOk;
  case ':':
  {
    int64_t v = 0;
    if (!ParseInt(data, line_begin, line_end, &v)) return kParseError;
    out->type = RedisReply::kInteger;
    out->integer = v;
    out->str.assign(data + line_begin, line_end - line_begin);
    *pos = after_line;
    return kParseOk;
  }
  case '$':
  {
    int64_t str_len = 0;
    if (!ParseInt(data, line_begin, line_end, &str_len)) return kParseError;
    if (str_len == -1) {
      out->type = RedisReply::kNil;
      *pos = after_line;
      return kParseOk;
    }
    if (str_len < 0 || str_len > kMaxBulkLen) return kParseError;
    size_t need = after_line + (size_t)str_len + 2;
    if (need > len) {
      if (need_hint) *need_hint = need;
      return kParseNeedMore;
    }
    if (data[need - 2] != '\r' || data[need - 1] != '\n') return kParseError;
    out->type = RedisReply::kString;
    out->str.assign(data + after_line, (size_t)str_len);
    *pos = need;
    return kParseOk;
  }
  case '*':
  {
    int64_t array_num = 0;
    if (!ParseInt(data, line_begin, line_end, &array_num)) return kParseError;
    if (array_num == -1) {
      out->type = RedisReply::kNil;
      *pos = after_line;
      return kParseOk;
    }
    if (array_num < 0 || array_num > kMaxArrayLen) return kParseError;
    out->type = RedisReply::kArray;
    out->elements.clear();
    size_t cur = after_line;
    for (int64_t i = 0; i < array_num; ++i) {
      // Need-more is detected before the array is materialized in full; do
      // not reserve array_num up front (it is attacker-controlled).
      out->elements.emplace_back();
      int ret = ParseElement(data, len, &cur, depth + 1, &out->elements.back(), need_hint);
      if (ret != kParseOk) return ret;
    }
    *pos = cur;
    return kParseOk;
  }
  default:
    return kParseError;
  }
}

int RedisParser::ParseReply(
  const char* data, size_t len, size_t* consumed, RedisReply* out,
  size_t* need_hint) {
  if (nullptr == data || nullptr == consumed || nullptr == out) return kParseError;
  if (0 == len) return kParseNeedMore;
  RedisReply result;
  size_t pos = 0;
  int ret = ParseElement(data, len, &pos, 0, &result, need_hint);
  if (ret != kParseOk) return ret;
  *consumed = pos;
  *out = std::move(result);
  return kParseOk;
}

bool RedisParser::EncodeArgs(const std::vector<std::string>& args, std::string& out) {
  if (args.empty()) return false;
  char int_buf[32];
  snprintf(int_buf, sizeof(int_buf), "%zu", args.size());
  out.assign("*").append(int_buf).append("\r\n");
  for (const auto& arg : args) {
    snprintf(int_buf, sizeof(int_buf), "%zu", arg.size());
    out.append("$").append(int_buf).append("\r\n").append(arg).append("\r\n");
  }
  return true;
}

bool RedisParser::ParseCmd(const std::string& in, std::string& out) {
  if (in.empty()) return false;
  // Split on kSep. Consecutive separators give an empty argument (the old
  // loop rejected them). One trailing kSep is ignored as before, so callers
  // that end every argument with kSep keep working; for an empty *last*
  // argument use EncodeArgs / RedisMessage::cmds.
  std::vector<std::string> args;
  size_t from = 0;
  while (true) {
    size_t pos = in.find(kSep, from);
    if (pos == std::string::npos) {
      args.emplace_back(in, from, std::string::npos);
      break;
    }
    args.emplace_back(in, from, pos - from);
    from = pos + 1;
  }
  if (args.size() > 1 && args.back().empty()) {
    args.pop_back(); // trailing separator
  }
  if (args[0].empty()) return false; // a command name is required
  return EncodeArgs(args, out);
}

}

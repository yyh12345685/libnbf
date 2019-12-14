
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

int RedisParser::ParseReply(
  const std::string& reply, bool* is_pong, size_t* size_reply, std::string* out) {
  if (0 == reply.length() || nullptr == is_pong || nullptr == size_reply || nullptr == out)
    return -1;

  size_t first_CRLF = reply.find("\r\n");
  if (first_CRLF == reply.npos){
    return -1;
  }

  const char* p = reply.c_str();
  *is_pong = false;
  //*size_reply = 0;

  switch (p[0]) {
  case '-'://REDIS_REPLY_ERROR
  {
    *size_reply = first_CRLF + 2;
    out->append("-1");
    return 0;
  }
  case '+'://REDIS_REPLY_STATUS
  {
    *size_reply = first_CRLF + 2;
    out->append("0");
    if (first_CRLF == 5 && reply.substr(1, 4) == "PONG") {
      *is_pong = true;
    }
    return 0;
  }
  case ':'://REDIS_REPLY_INTEGER
  {
    *size_reply = first_CRLF + 2;
    out->push_back('1');
    out->push_back(kSep);
    out->append(reply.substr(1, first_CRLF-1));
    return 0;
  }
  case '$'://REDIS_REPLY_STRING
  {
    std::string tmp_out;
    int ret = ParseSingleStr(p, reply.size(), *size_reply, tmp_out);
    if (0 == ret){
      out->push_back('1');
      out->push_back(kSep);
      out->append(tmp_out);
      return 0;
    } else if (1 == ret){
      out->append("0");
      return 0;
    } else if (2 == ret){
      return 1;
    } else{
      return -1;
    }
  }
  case '*'://REDIS_REPLY_ARRAY
  {
    *size_reply = first_CRLF + 2;
    char* end_ptr = nullptr;
    int64_t array_num = strtoll(++p, &end_ptr, 10);
    if (array_num > 0){
      *out = reply.substr(1,first_CRLF-1);
      for (int i = 0; i < array_num;++i){
        std::string tmp_out;
        size_t one_str_len = 0;
        int ret = ParseSingleStr(
          reply.c_str() + *size_reply, reply.size() - *size_reply, one_str_len, tmp_out);
        if (0 == ret){
          out->push_back(kSep);
          out->append(tmp_out);
        }else if (1 == ret){
          out->push_back(kSep);
          out->append("-1");
        }else if (2 == ret){
          return 1;
        }else{
          return -1;
        }
        *size_reply += one_str_len;
      }
      return 0;
    }
    else if (array_num == -1){
      out->append("0");
      return 0;
    }
    else{
      return -1;
    }
  }
  default:
    return -1;
  }
}

bool RedisParser::ParseCmd(const std::string& in, std::string& out) {
  if (0==in.length()) return false;

  std::string result("\r\n");
  size_t follower=0;
  size_t leader;
  uint32_t num_segs=0;

  static const size_t kMaxLenInt64 = 30;
  char int_buf[kMaxLenInt64 + 1] = {'\0'};
  do{
    leader = in.find(kSep, follower);
    if (leader == follower)
    {//跳过连续的\1
      //follower = leader + 1;
      //continue;
      return false;
    }

    uint32_t len_seg = (std::string::npos!=leader ? leader-follower : in.length()-follower);
    if (0==len_seg) break;

    ++num_segs;

    sprintf(int_buf, "%u", len_seg);
    result.append("$").append(int_buf).append("\r\n").append(in, follower, len_seg).append("\r\n");

    follower=leader+1;
  } while(leader!=std::string::npos);

  sprintf(int_buf, "%u", num_segs);
  out.assign("*").append(int_buf).append(result);
  return true;
}

int RedisParser::ParseSingleStr(const char* line_data, const size_t& line_len, size_t& has_read_len, std::string& out){
  if (line_data[0] == '$'){
    int length_line_len = GetLengthLinelength(line_data, line_len);
    if (length_line_len == -1){
      return -1;
    }
    has_read_len += length_line_len;
    char* read_pos = nullptr;
    int64_t str_len = strtoll(&line_data[1], &read_pos, 10);
    if (str_len > 0){
      read_pos += 2;//jump \r\n
      if (str_len <= (signed)(line_len - has_read_len - 2))//-2 include \r\n
      {
        if (read_pos[str_len] == '\r' && read_pos[str_len + 1] == '\n'){
          out.assign(read_pos, str_len);
          has_read_len += str_len + 2;
          return 0;
        }else{
          return -1;//wrong end, missing \r\n
        }
      }else{
        return 2;//need to recv the rest string
      }
    }else if (str_len == -1){
      return 1;
    }else{
      return -1;
    }
  }else{
    return -1;
  }
}

}


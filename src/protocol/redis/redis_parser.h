
#pragma once

#include <string>

namespace bdf {

class RedisParser {
public:
  static const char kSep = '\1';

public:
  //return -1:err, 0:ok, 1:need to recv rest
  static int ParseReply(
    const std::string& reply, bool* is_pong, size_t* size_reply, std::string* out);

  static bool ParseCmd(const std::string& in, std::string& out);

private: 
  // return 0:ok, 1:get empty, 2:socket recv may not complete
  static int ParseSingleStr(
    const char* line_data, const size_t& line_len, size_t& has_read_len, std::string& out);

  inline static int GetLengthLinelength(const char* line_data, const size_t& line_len){
    size_t pos = 1;//pass '$'
    size_t safe_len = line_len - 1;//the last need has \r\n 2 bytes
    while (pos < safe_len){
      while (pos < safe_len && line_data[pos] != '\r') ++pos;
      if (pos == safe_len){
        //not find \r\n
        return -1;
      }else{
        if (line_data[pos + 1] == '\n'){
          //find \r\n
          return pos + 2;//include \r\n
        }
        else{
          ++pos;//continue find
        }
      }
    }
    return -1;
  }
};

}


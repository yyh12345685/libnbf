#pragma once

#include "message_base.h"
#include "protocol/http/http_info.h"

namespace bdf {

struct HeartBeatMessage : public EventMessage {
  HeartBeatMessage() :
    EventMessage(MessageType::kHeartBeatMessage) {
  }
  virtual ~HeartBeatMessage() {
  }
  virtual bool IsSynchronous() { return true; }
  uint64_t load_avg;
  int heart_type;
};

template<class Stream>
Stream& operator << (Stream &os, const HeartBeatMessage &msg){
  os << "HeartBeatEvent, load_avg: " << msg.load_avg << ", ";
  os << "heart_type: " << msg.heart_type;
  return os;
}

struct RapidMessage : public EventMessage {
  RapidMessage() :
    EventMessage(MessageType::kRapidMessage), command(0) {
  }
  virtual ~RapidMessage() {
  }
  virtual bool IsSynchronous() { return false; }

  void Dump(std::ostream& os) const{
    os << "{\"type\": \"RapidMessage\""
      << ", \"EventMessage\": ";
    EventMessage::Dump(os);
    os << ", \"command\": " << command
      << ", \"body\": \"" << body << "\""
      << "}";
  }

  std::string body;
  int command;
};

template<class Stream>
Stream& operator << (Stream& os, const RapidMessage& message){
  message.Dump(os);
  return os;
}

struct RedisMessage : public EventMessage {
  RedisMessage() :
    EventMessage(MessageType::kRedisMessage) {
  }
  virtual ~RedisMessage() {
  }
  void Dump(std::ostream& os) const {
    os << "{\"type\": \"RapidMessage\""
      << ", \"EventMessage\": ";
    EventMessage::Dump(os);
    os << ", \"bodys\": " << bodys.size()<< "\""
      << "}";
  }
  virtual bool IsSynchronous() { return true; }
  std::vector<std::string> bodys;
};

template<class Stream>
Stream& operator << (Stream& os, const RedisMessage& message) {
  message.Dump(os);
  return os;
}

struct HttpMessage : public EventMessage {
  HttpMessage() :
    EventMessage(MessageType::kHttpMessage) {
  }
  virtual ~HttpMessage() {
  }
  uint16_t http_major;
  uint16_t http_minor;
  uint16_t status_code;
  bool keep_alive;
  bool encode_gzip;

  HttpRequestInfo http_info;

  virtual bool IsSynchronous() { return true; }

  inline size_t GetHeaderCount() const{
    return http_info.headers.size();
  }

  inline void ClearHeader(){
    http_info.headers.clear();
  }

  inline bool GetHeader(const std::string& field, std::string *value){
    const auto& it = http_info.headers.find(field);
    if (it != http_info.headers.end()){
      if (value != nullptr){
        *value = it->second;
      }
      return true;
    }
    return false;
  }

  inline void InitReply(HttpMessage *ev,
    uint16_t status, bool encode_gzip = false, bool keep_alive = true){
    this->http_major = ev ? ev->http_major : 1;
    this->http_minor = ev ? ev->http_minor : (keep_alive ? 1 : 0);
    this->status_code = status;
    this->keep_alive = (!ev || ev->keep_alive) && keep_alive;
    this->encode_gzip = (!ev || ev->encode_gzip) && encode_gzip;
    this->direction = MessageBase::kIncomingResponse;
  }

  inline void InitRequest(const char *cmd,
    bool http_1_1, bool encode_gzip = false, bool keep_alive = true){
    this->http_major = 1;
    this->http_minor = http_1_1 ? 1 : 0;
    this->status_code = 0;
    this->keep_alive = keep_alive;
    this->encode_gzip = encode_gzip;
    this->http_info.method = cmd;
    this->direction = MessageBase::kOutgoingRequest;
  }
protected:
  HttpMessage(int type) :EventMessage(type) {}
};

/// helper for stream-style output
template<class Stream>
Stream& operator << (Stream &os, const HttpMessage &msg){
  os << "HTTP " << msg.http_major << "." << msg.http_minor << ", ";
  os << "status_code: " << msg.status_code << ", ";
  os << "method: " << msg.http_info.method << ", ";
  os << "keep_alive: " << msg.keep_alive << ", ";
  os << "url: " << msg.http_info.url << ", ";
  for (const auto& it : msg.http_info.headers){
    os << it.first << "=" << it.second << ", ";
  }
  os << "body.size(): " << msg.http_info.body.size() << ", ";
  os << "body: " << msg.http_info.body;
  return os;
}

}

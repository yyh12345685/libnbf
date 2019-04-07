
#pragma once

#include <stdint.h>
#include <sstream>
#include "monitor/mem_profile.h"

namespace bdf {

class MessageType {
 public:
  enum {
    kUnknownEvent = 0,
    kHeartBeatMessage,
    kRapidMessage,
    kHttpMessage,
    kRedisMessage,
  };

  enum {
    kIoUnknownEvent = 0,
    kIoMessageEvent,
    kIoActiveCloseEvent,
  };

  static const char* ToString(int type) {
    static const char* str[] = {
      "unknown event",
      "heart beat message",
      "rapid protocol message",
      "http protocol message",
      "redis protocol message",
    };

    if (type < 0 || type >= (int)(sizeof(str) / sizeof(const char*))) {
      return "unknown event";
    }

    return str[type];
  }

  static const char* ToIoEventString(int type) {
    static const char* str[] = {
      "unknown io event",
      "io message event",
      "io active close event",
    };

    if (type < 0 || type >= (int)(sizeof(str) / sizeof(const char*))) {
      return "unknown event";
    }

    return str[type];
  }
};

class MessageBase {
public:
  enum{
    kDirectionUnknown = 0,
    kIncomingRequest,//server receive msg
    kOutgoingRequest,//client send msg
    kIncomingResponse,//client response msg
    kOutgoingResponse,//server response msg
    //1：只是客户端单项发送，服务端不应答
    kOnlySend,//client only send，no response
    //2:客户端发送，不关心是否有返回，服务端可能返回也可能不返回(一般有返回)
    //目前的代码来看，同步客户端会出现2的情况(AsyncClientConnect)，异步客户端会出现1的情况(SyncClientConnect)
    kSendNoCareResponse
  };
  enum{
    kStatusOK = 0,
    kStatusTimeout,
    kStatusEncodeFail,
    kInternalFailure,
  };
public:

  MessageBase(uint8_t type_id) :
    type_id(type_id),
    type_io_event(0){
  }

  virtual ~MessageBase() {};

  std::string ToString() const;
  void Dump(std::ostream& os) const;

  uint8_t type_id;
  uint8_t type_io_event;
  uint8_t direction = 0;
  uint8_t status = kStatusOK;
  uint64_t sequence_id=0xffffffffffffffff;
  uint64_t birthtime;
};

class EventMessage : public MessageBase {
 public:
  EventMessage(uint8_t type_id): 
    MessageBase(type_id), 
    descriptor_id(0),
    handle_id(-1){
  }

  EventMessage(uint8_t type_id_, int64_t descriptor_id_) : 
    MessageBase(type_id_), 
    descriptor_id(descriptor_id_){
  }

  virtual ~EventMessage() {}

  virtual bool IsSynchronous() { return false; }

  void Dump(std::ostream& os) const;

  void SetDescriptorId(const int64_t& desc_id){
    descriptor_id = desc_id;
  }

  int64_t GetDescriptorId() {
    return descriptor_id;
  }

  int64_t descriptor_id;//convert object
  int32_t handle_id;
};

class MessageFactory {
 public:
  template<typename MessageType, typename... Args>
  static MessageType* Allocate(const Args&... args) {
    return BDF_NEW(MessageType, args...);
  }

  static void Destroy(MessageBase* message) {
    BDF_DELETE(message);
  }
};

std::ostream& operator << (std::ostream& os, const EventMessage& message);

}

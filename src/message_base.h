
#pragma once

#include <stdint.h>
#include <sstream>
#include "monitor/mem_profile.h"

namespace bdf {

class ProtocolMessage;

class MessageType {
 public:
  enum {
    kUnknownEvent = 0,
    kHeartBeatMessage,
    kRapidMessage,
    kHttpMessage,
    kRedisMessage,
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
};

class MessageBase {
public:
  enum{
    kDirectionUnknown = 0,
    kIncomingRequest,
    kOutgoingRequest,
    kIncomingResponse,
    kOutgoingResponse,
    kOneway,
  };
  enum{
    kStatusOK = 0,
    kStatusTimeout,
    kStatusEncodeFail,
    kInternalFailure,
  };
public:

  MessageBase(uint8_t type_id) 
    : type_id(type_id) {
  }

  virtual ~MessageBase() {};

  std::string ToString() const;
  virtual void Dump(std::ostream& os) const;

  uint8_t type_id;
  uint8_t direction;
  uint8_t status;
  uint64_t sequence_id;
};

class EventMessage : public MessageBase {
 public:
  EventMessage() 
      : MessageBase(MessageType::kUnknownEvent), descriptor_id(0) {
  }

  EventMessage(
      uint8_t type_id_, 
      uint64_t descriptor_id_) 
      : MessageBase(type_id_), 
      descriptor_id(descriptor_id_){
  }

  virtual void Dump(std::ostream& os) const;

  uint64_t descriptor_id;//convert object
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

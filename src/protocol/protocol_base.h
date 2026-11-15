
#pragma once
#include <vector>
#include "message_base.h"

namespace bdf {

class EventMessage;
class Buffer;

class ProtocolBase {
public:
  ProtocolBase(){}
  virtual ~ProtocolBase() {}
public:
  virtual bool IsSynchronous() = 0;
  virtual void Release() = 0;
  virtual EventMessage *Decode(Buffer &input, bool &failed) = 0;
  virtual bool Encode(EventMessage *msg, Buffer *output) = 0;
  virtual ProtocolBase* Clone() = 0;

  virtual EventMessage* HeartBeatRequest() = 0;
  virtual EventMessage* HeartBeatResponse(EventMessage* request) = 0;

  // Drop per-connection parse state. Called when the connection's socket is
  // closed (Connecting::Clean): a client reconnects with the same protocol
  // object and must not inherit a half-parsed message or parser error.
  virtual void Reset() {}
};

class ProtocolFactory {
public:
  ProtocolFactory();

  ~ProtocolFactory();

  ProtocolBase* Create(int type);

private:
  LOGGER_CLASS_DECL(logger);

  void RegisterProtocol(int type, ProtocolBase* protocol);

  std::vector<ProtocolBase*> protocol_;
};

}

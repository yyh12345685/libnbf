
#pragma once

#include <string.h>
#include <stdlib.h>

#include "protocol/protocol_base.h"
#include "common/logger.h"
#include "monitor/mem_profile.h"

namespace bdf {

class EventMessage;
class Buffer;

class RapidProtocol : public ProtocolBase {
public:
  RapidProtocol(){}
  virtual ~RapidProtocol(){}

  virtual bool IsSynchronous() {
    return false; 
  }

  virtual EventMessage* Decode(Buffer &input, bool& failed);
  virtual bool Encode(EventMessage *msg, Buffer *output);

  virtual ProtocolBase* Clone() { 
    return BDF_NEW (RapidProtocol);
  }

  virtual void Release() {
    BDF_DELETE(this);
  }

  virtual EventMessage* HeartBeatRequest();
  virtual EventMessage* HeartBeatResponse(EventMessage* request);
 private:
  LOGGER_CLASS_DECL(logger_);
};

}

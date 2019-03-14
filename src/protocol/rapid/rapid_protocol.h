
#pragma once

#include <string.h>
#include <stdlib.h>

#include "protocol/protocol_base.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;
class Buffer;

class RapidProtocol : public ProtocolBase {
public:
  RapidProtocol(){}
  virtual ~RapidProtocol(){}
  virtual bool IsSynchronous() { return false; }
  virtual EventMessage* Decode(Buffer &input, bool& failed);
  virtual bool Encode(EventMessage *msg, Buffer *output);

  virtual ProtocolBase* Clone() { return new RapidProtocol(); }
  virtual void Release() { delete this; };
 private:
  LOGGER_CLASS_DECL(logger_);
};

}


#pragma once

#include <string.h>
#include <stdlib.h>

#include "protocol/protocol_base.h"
#include "common/logger.h"

namespace bdf {

class MessageBase;
class Buffer;

class RapidProtocol : public ProtocolBase {
public:
  RapidProtocol(){}
  virtual ~RapidProtocol(){}
  virtual bool IsSynchronous() { return false; }
  virtual MessageBase* Decode(Buffer &input, bool& failed);
  virtual bool Encode(MessageBase *msg, Buffer *output);
 private:
  LOGGER_CLASS_DECL(logger_);
};

}

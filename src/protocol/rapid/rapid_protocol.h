
#pragma once

#include <string.h>
#include <stdlib.h>

#include "protocol/protocol_base.h"
#include "common/logger.h"

namespace bdf {

class MessageBase;
namespace common {
  class Buffer;
}

namespace protocol {

class RapidProtocol : public ProtocolBase {
public:
  RapidProtocol(){}
  virtual ~RapidProtocol(){}
  virtual bool IsSynchronous() { return false; }
  virtual MessageBase* Decode(common::Buffer &input, bool& failed);
  virtual bool Encode(MessageBase *msg, common::Buffer *output);
 private:
  LOGGER_CLASS_DECL(logger_);
};

}

}


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

class RedisProtocol : public ProtocolBase {
public:
  RedisProtocol(){}
  virtual ~RedisProtocol(){}
  virtual bool IsSynchronous() { return true; }
  virtual MessageBase *Decode(common::Buffer &input, bool& failed);
  virtual bool Encode(MessageBase *msg, common::Buffer *output);
private:
  LOGGER_CLASS_DECL(logger_);
};

}
}

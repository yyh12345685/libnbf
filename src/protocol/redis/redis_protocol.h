
#pragma once

#include <string.h>
#include <stdlib.h>
#include <deque>
#include <string>
#include <vector>
#include "protocol/redis/redis_reply.h"

#include "protocol/protocol_base.h"
#include "common/logger.h"
#include "monitor/mem_profile.h"

namespace bdf {

class EventMessage;
class Buffer;

class RedisProtocol : public ProtocolBase {
public:
  RedisProtocol(){}
  virtual ~RedisProtocol(){}

  virtual bool IsSynchronous() {
    return true; 
  }

  virtual EventMessage *Decode(Buffer &input, bool& failed);
  virtual bool Encode(EventMessage *msg, Buffer *output);

  virtual ProtocolBase* Clone() {
    return BDF_NEW (RedisProtocol);
  }

  virtual void Release() {
    BDF_DELETE(this);
  };

  virtual EventMessage* HeartBeatRequest();
  virtual EventMessage* HeartBeatResponse(EventMessage* request);

  virtual void Reset();
private:
  LOGGER_CLASS_DECL(logger_);

  // Replies still expected per encoded request, in send order. A request
  // with N commands gets N replies; Decode merges them into one message so
  // the connection's one-request-one-response matching stays aligned.
  // Encode and Decode both run on the connection's I/O thread.
  std::deque<size_t> expected_replies_;
  std::vector<RedisReply> partial_replies_;
  // Bytes the incomplete reply at the buffer head needs at least; parsing is
  // skipped until that much has arrived (a large reply used to be re-parsed
  // from scratch on every read).
  size_t need_bytes_ = 0;
};

}

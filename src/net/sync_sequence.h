#pragma once

#include <queue>
#include "common/logger.h"

namespace bdf {

class MessageBase;

class SyncSequence {
public:
  SyncSequence();
  ~SyncSequence();

  int Put(MessageBase* message);
  MessageBase* Get(uint32_t sequence_id);

  std::queue<MessageBase*> Timeout();

private:
  uint32_t timeout_ms_;
  std::queue<MessageBase*> queue_;

  LOGGER_CLASS_DECL(logger);

};

}

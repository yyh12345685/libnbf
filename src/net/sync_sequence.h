#pragma once

#include <queue>
#include "common/logger.h"

namespace bdf {

class EventMessage;

class SyncSequence {
public:
  SyncSequence();
  ~SyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get(uint32_t sequence_id);

  std::queue<EventMessage*> Timeout();

private:
  uint32_t timeout_ms_;
  std::queue<EventMessage*> queue_;

  LOGGER_CLASS_DECL(logger_);

};

}

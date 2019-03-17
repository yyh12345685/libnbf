#pragma once

#include <unordered_map>
#include <queue>
#include <atomic>
#include "common/logger.h"

namespace bdf {

class EventMessage;

class AsyncSequence{
public: 
  AsyncSequence();
  virtual ~AsyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get(uint32_t sequence_id);

  std::queue<EventMessage*> Timeout();

  inline uint32_t GenerateSequenceId() { 
    return ++sequence_id_;
  }

private:
  std::atomic<uint32_t> sequence_id_;
  uint32_t timeout_ms_;

  std::unordered_map<uint32_t, EventMessage*> registery_;

  std::queue<EventMessage*> queue_;

  LOGGER_CLASS_DECL(logger);
};

}


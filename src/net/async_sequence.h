#pragma once

#include <unordered_map>
#include <list>
#include <atomic>
#include "common/logger.h"

namespace bdf {

class EventMessage;

class AsyncSequence{
public: 
  AsyncSequence(uint32_t timeout_in_ms);
  virtual ~AsyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get(uint32_t sequence_id);

  std::list<EventMessage*> Timeout();
  std::list<EventMessage*> Clear();

  inline uint32_t GenerateSequenceId() { 
    return ++sequence_id_;
  }

private:
  std::atomic<uint32_t> sequence_id_;
  uint32_t timeout_ms_;

  std::unordered_map<uint32_t, EventMessage*> registery_;

  std::list<EventMessage*> list_;

  LOGGER_CLASS_DECL(logger);
};

}


#pragma once

#include <unordered_map>
#include <queue>
#include <atomic>
#include "common/logger.h"

namespace bdf {

class MessageBase;

class AsyncSequence{
public: 
  AsyncSequence();
  ~AsyncSequence();

  int Put(MessageBase* message);
  MessageBase* Get(uint32_t sequence_id);

  std::queue<MessageBase*> Timeout();

  inline uint32_t GenerateSequenceId() { 
    return ++sequence_id_;
  }

private:
  std::atomic<uint32_t> sequence_id_;
  uint32_t timeout_ms_;

  std::unordered_map<uint32_t, MessageBase*> registery_;

  std::queue<MessageBase*> queue_;

  LOGGER_CLASS_DECL(logger);
};

}


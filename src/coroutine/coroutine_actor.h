#pragma once

#include <list>
#include "coroutine/coroutine.h"
#include "common/logger.h"

namespace bdf{

class EventMessage;

class CoroutineActor:public CoroContext {
public:
  CoroutineActor():
    is_waiting_(false),
    waiting_id_(-1){
    msg_list_.clear();
  }

  EventMessage* RecieveMessage(EventMessage* message,uint32_t timeout_ms = 0);
  bool SendMessage(EventMessage* message);

  inline void SetWaitingId(const uint64_t& sequence_id) {
    is_waiting_ = true;
    waiting_id_ = sequence_id;
  }
private:
  LOGGER_CLASS_DECL(logger_);

  bool is_waiting_;
  uint64_t waiting_id_;
  std::list<EventMessage*> msg_list_;
};

}


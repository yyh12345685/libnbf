#pragma once

#include <list>
#include "coro_context_c.h"
#include "common/logger.h"
#include "coroutine/coroutine_actor.h"

namespace bdf{

class EventMessage;

class CoroutineActorc : public CoroutineActor {
public:
  CoroutineActorc();
  ~CoroutineActorc() override;

  EventMessage* RecieveMessage(uint64_t sequence_id, uint32_t timeout_ms = 0) override;
  virtual bool SendMessage(EventMessage* message);

  virtual void SetWaitingId(const uint64_t& sequence_id) {
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


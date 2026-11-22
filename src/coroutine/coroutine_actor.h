#pragma once

#include <stdint.h>

namespace bdf {

  class EventMessage;

  class CoroutineActor {
  public:
    virtual ~CoroutineActor(){}

    virtual EventMessage* RecieveMessage(uint64_t sequence_id, uint32_t timeout_ms = 0) = 0;
    virtual bool SendMessage(EventMessage* message) = 0;

    virtual void SetWaitingId(const uint64_t& sequence_id) = 0;
  };

}


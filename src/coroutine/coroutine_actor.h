#pragma once

#include <list>

namespace bdf {

  class EventMessage;

  class CoroutineActor {
  public:

    virtual EventMessage* RecieveMessage(EventMessage* message, uint32_t timeout_ms = 0) = 0;
    virtual bool SendMessage(EventMessage* message) = 0;

    virtual void SetWaitingId(const uint64_t& sequence_id) = 0;
  };

}


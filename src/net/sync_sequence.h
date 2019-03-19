#pragma once

#include <list>
#include "common/logger.h"

namespace bdf {

class EventMessage;

class SyncSequence {
public:
  SyncSequence(uint32_t timeout_in_ms);
  virtual ~SyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get();

  EventMessage* Fired();
  EventMessage* Fire();

  std::list<EventMessage*> Timeout();
  std::list<EventMessage*> Clear();
private:
  uint32_t timeout_ms_;
  std::list<EventMessage*> list_;

  EventMessage* fired_;

  LOGGER_CLASS_DECL(logger_);

};

}

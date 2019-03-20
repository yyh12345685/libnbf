#pragma once

#include <list>
#include "common/logger.h"
#include "event/timer/timer_base.h"

namespace bdf {

class EventMessage;
class SyncClientConnect;

class SyncSequence :public OnTimerBase {
public:
  SyncSequence(SyncClientConnect* sync_client_con,uint32_t timeout_in_ms);
  virtual ~SyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get();

  EventMessage* Fired();
  EventMessage* Fire();

  virtual void OnTimer(void* function_data);
  std::list<EventMessage*> Clear();
private:
  SyncClientConnect* sync_client_con_;
  uint32_t timeout_ms_;
  std::list<EventMessage*> list_;

  EventMessage* fired_;

  LOGGER_CLASS_DECL(logger_);

};

}

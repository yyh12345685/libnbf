#pragma once

#include <unordered_map>
#include <list>
#include <mutex>
#include "common/logger.h"
#include "event/timer/timer_base.h"

namespace bdf {

class EventMessage;
class AsyncClientConnect;

class AsyncSequence:public OnTimerBase {
public: 
  AsyncSequence(AsyncClientConnect* async_client_con,uint32_t timeout_in_ms);
  virtual ~AsyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get(uint64_t sequence_id);

  int GetSequenceSize() { return registery_.size(); }

  virtual void OnTimer(void* function_data);

  std::list<EventMessage*> Clear();

  uint64_t StartTimer(void* data);
  void CancelTimer(uint64_t timer_id);

private:
  AsyncClientConnect* async_client_con_;
  uint32_t timeout_ms_;

  std::unordered_map<uint64_t, EventMessage*> registery_;
  //如果这个类只和service handle的线程有关系，和io handle线程无关，则可无锁
  //应该可以优化
  std::mutex registery_lock_;

  std::list<EventMessage*> list_;
  //如果这个类只和service handle的线程有关系，和io handle线程无关，则可无锁
  //应该可以优化
  std::mutex list_lock_;

  LOGGER_CLASS_DECL(logger);
};

}


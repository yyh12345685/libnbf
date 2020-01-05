#pragma once

#include <list>
#include <mutex>
#include "common/logger.h"
#include "event/timer/timer.h"

namespace bdf {

class EventMessage;
class SyncClientConnect;

class SyncSequence :public OnTimerBase {
public:
  SyncSequence(SyncClientConnect* sync_client_con,uint32_t timeout_in_ms);
  virtual ~SyncSequence();

  int Put(EventMessage* message);
  EventMessage* Get();

  int Size(){
    return list_.size();
  }

  virtual void OnTimer(void* function_data);
  std::list<EventMessage*> Clear();
  void CancelTimer(uint64_t timer_id);
  
private:
  SyncClientConnect* sync_client_con_;
  uint32_t timeout_ms_;
  std::list<EventMessage*> list_;

  uint64_t StartTimer();

  LOGGER_CLASS_DECL(logger_);

  //如果这个类只和service handle的线程有关系，和io handle线程无关，则可无锁
  //应该可以优化
  std::mutex lock_;

};

}

#pragma once

#include <iostream>
#include "coroutine/coroutine_schedule.h"

namespace bdf {

struct CoroutineActor;
class CoroutineSchedule;
class CoroutineServiceHandler;
class Timer;

struct CoroutineContext {
 public:

  static CoroutineContext& Instance() {
    static thread_local CoroutineContext context_;
    return context_;
  }

  static void Init(CoroutineServiceHandler* service_handle,Timer* timer);

  inline static CoroutineActor* GetCoroutine() {
    return Instance().coroutine_;
  }
  inline static CoroutineSchedule* GetScheduler() {
    return Instance().scheduler_;
  }
  inline static Timer* GetTimer() {
    return Instance().timer_;
  }
  inline static CoroutineServiceHandler* GetServiceHandler() {
    return Instance().service_handle_;
  }

 private:
  CoroutineContext(){
  }

  ~CoroutineContext() {
    if (nullptr != scheduler_ && nullptr != coroutine_){
      scheduler_->CoroutineClose(coroutine_);
      delete scheduler_;
    }
  }

  CoroutineSchedule* scheduler_;
  CoroutineActor* coroutine_;
  CoroutineServiceHandler* service_handle_;
  Timer* timer_;
};

}

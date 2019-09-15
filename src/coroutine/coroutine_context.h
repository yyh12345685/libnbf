#pragma once

#include <iostream>
#include "coroutine/coroutine_schedule.h"

namespace bdf {

struct CoroutineActor;
class CoroutineSchedule;
class CoroutineServiceHandler;
class TimerMgr;

struct CoroutineContext {
 public:

  static CoroutineContext& Instance() {
    static thread_local CoroutineContext context_;
    return context_;
  }

  static void Init(
    CoroutineServiceHandler* service_handle,
    TimerMgr* time_mgr);

  inline static CoroutineActor* GetCurCoroutineCtx() {
    return Instance().scheduler_->GetCoroutineCtx(Instance().GetCurCoroutineId()) ;
  }

  inline static int GetCurCoroutineId() {
    return Instance().cur_coroutine_id_;
  }

  inline static void SetCurCoroutineId(int cur_coroutine_id) {
    Instance().cur_coroutine_id_ = cur_coroutine_id;
  }

  inline static CoroutineSchedule* GetScheduler() {
    return Instance().scheduler_;
  }

  inline static TimerMgr* GetTimerMgr() {
    return Instance().time_mgr_;
  }

  inline static CoroutineServiceHandler* GetServiceHandler() {
    return Instance().service_handle_;
  }

 private:
  CoroutineContext(){
  }

  ~CoroutineContext() {
    if (nullptr != scheduler_){
      delete scheduler_;
    }
  }

  CoroutineSchedule* scheduler_ = nullptr;
  int cur_coroutine_id_ = -1;
  CoroutineServiceHandler* service_handle_ = nullptr;
  TimerMgr* time_mgr_ = nullptr;
};

}

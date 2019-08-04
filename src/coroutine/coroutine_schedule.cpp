#include "coroutine_schedule.h"
#include "coroutine_actor.h"
#include "coroutine_context.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineSchedule);

CoroutineSchedule::~CoroutineSchedule(){
  if (nullptr != coro_sche_){
    coro_impl_.CoroutineClose(coro_sche_);
  }
}

void CoroutineSchedule::InitCoroSchedule(CoroutineFunc func, void* data){
  coro_sche_ = coro_impl_.CoroutineInit();
  bool is_first = true;
  for (int idx = 0; idx < coro_sche_->cap;idx++) {
    int coroutine_id = coro_impl_.CoroutineNew(coro_sche_, func, data);
    all_coro_list_.emplace_back(coroutine_id);
    available_coro_list_.insert(coroutine_id);
    if (is_first){
      is_first = false;
    }
  }
}

int CoroutineSchedule::GetAvailableCoroId(){
  for (const auto id: available_coro_list_){
    int status = CoroutineStatus(id);
    if (CoroutineImpl::kCoroutineReady == status
      || CoroutineImpl::kCoroutineSuspend == status) {
      return id;
    }
    return id;
  }

  WARN(logger_, "no available coro:" << available_coro_list_.size());
  return -1;

}

void CoroutineSchedule::CoroutineYield(int coro_id){
  available_coro_list_.erase(coro_id);
  if (coro_id == GetRunningId()){
    TRACE(logger_, "will be yield coroutine:"<< coro_id);
    CoroutineContext::SetCurCoroutineId(-1);
    CoroutineYield();
  } else {
    WARN(logger_, "coro_id:" << coro_id<<",GetRunningId()"<< GetRunningId());
  }
}

void CoroutineSchedule::CoroutineYieldToActive(int coro_id){
  //说明要切换协程，切换回来之后可能是收到了客户端的消息或者超时
  active_coro_list_.push(coro_id);
  if (GetRunningId() >= 0) {
    //当前是在协程中，切出去
    TRACE(logger_, "cur coro id:" << GetRunningId() << ",change to coro id:" << coro_id);
    CoroutineContext::SetCurCoroutineId(-1);
    CoroutineYield();
  }
}

CoroutineActor* CoroutineSchedule::GetCoroutineCtx(int id){
  return coro_impl_.GetCoroutineCtx(coro_sche_,id);
}

//启动协程或者恢复有返回的协程
bool CoroutineSchedule::CoroutineResumeActive(){
  if (GetRunningId() >= 0){
    INFO(logger_, "not to here,cur coro id:" << GetRunningId());
    return false;
  }
  if (active_coro_list_.size()>0){
    int id = active_coro_list_.front();
    TRACE(logger_, "change to coro id:" << id);
    active_coro_list_.pop();
    CoroutineResume(id);
    return true;
  }
  return false;
}

void CoroutineSchedule::CoroutineResume(int id) {
  //if (GetRunningId() >= 0) {
  //  //如果当前是在协程中，先切出去
  //  INFO(logger_, "cur coro id:" << GetRunningId() << ",change to coro id:" << id);
  //  CoroutineContext::SetCurCoroutineId(-1);
  //  CoroutineYield();
  //}
  CoroutineContext::SetCurCoroutineId(id);
  available_coro_list_.insert(id);
  TRACE(logger_, "will be resume coroutine:" << id);
  coro_impl_.CoroutineResume(coro_sche_, id);
}

int CoroutineSchedule::GetRunningId(){
  return coro_impl_.CoroutineRunning(coro_sche_);
}

int CoroutineSchedule::CoroutineStatus(int id) {
  return coro_impl_.CoroutineStatus(coro_sche_, id);
}

void CoroutineSchedule::CoroutineYield() {
  coro_impl_.CoroutineYield(coro_sche_);
}

}

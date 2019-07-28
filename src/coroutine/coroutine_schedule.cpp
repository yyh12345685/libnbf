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
  }

  WARN(logger_, "no available coro:" << available_coro_list_.size());
  return -1;

}

void CoroutineSchedule::CoroutineYield(int coro_id){

  task_coro_list_.erase(coro_id);
  available_coro_list_.insert(coro_id);

  if (coro_id == GetRunningId()){
    TRACE(logger_, "will be yield coroutine:"<< coro_id);
    CoroutineYield();
  } else {
    TRACE(logger_, "coro_id:" << coro_id<<",GetRunningId()"<< GetRunningId());
  }
}

CoroutineActor* CoroutineSchedule::GetCoroutineCtx(int id){
  return coro_impl_.GetCoroutineCtx(coro_sche_,id);
}

void CoroutineSchedule::CoroutineResume(int id) {
  //说明要切换协程，切换回来之后可能是收到了客户端的消息或者超市
  if (GetRunningId() >= 0) {
    //如果当前是在协程中，先切出去
    CoroutineYield();
  }
  CoroutineContext::SetCurCoroutineId(id);
  task_coro_list_.insert(id);
  available_coro_list_.erase(id);
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

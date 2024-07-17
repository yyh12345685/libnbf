#include "coroutine_schedule.h"
#include "coroutine_actor.h"
#include "coroutine_manager.h"
#include "common/thread_id.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineSchedule);

CoroutineSchedule::~CoroutineSchedule(){
  coro_impl_.Release();
}

void CoroutineSchedule::InitCoroSchedule(
  CoroutineFunc func, void* data, int coroutine_size){

  coro_impl_.CoroutineInit(func, data, free_list_, coroutine_size);
}

CoroContext* CoroutineSchedule::GetAvailableCoro(){
  CoroContext* ret = nullptr;
  if (!free_list_.empty()) {
    // 从空闲队列中获取
    ret = free_list_.front();
    free_list_.pop();
  } else {
    // TODO,协程不够用，动态创建。
  }
  return ret;
  /*for (const auto id: available_coro_list_){
    return id;
  }

  //协程不够用，动态增加 TODO
  int id = AddNewCoroutine();
  if (id >= 0){
    INFO(logger_, "ThreadId:" << ThreadId::Get() << ",AddNewCoroutine id:" << id);
    return id;
  }

  WARN(logger_, "ThreadId:" << ThreadId::Get() 
    << ",no available coro,and add failed,coro size:" << available_coro_list_.size());
  return -1;*/
}

CoroContext* CoroutineSchedule::GetCurrentCoroutine() {
  return coro_impl_.GetCurrentCoroutine();
}

void CoroutineSchedule::ProcessDebug(){
  static thread_local time_t now = time(NULL);
  //static thread_local time_t now_1 = now;
  time_t cur_time = time(NULL);
  if ((cur_time - now) > 60) {
    //60秒一次trace
    INFO(logger_, "ThreadId:" << ThreadId::Get()
      << ",free list size:" << free_list_.size());
    now = cur_time;
    //for (size_t idx = 0; idx < yield_time_debug_.size(); idx++) {
    //  if (yield_time_debug_[idx].is_yield) {
    //    int yield_times = cur_time - yield_time_debug_[idx].yield_time;
    //    if (yield_times >= 10) {
    //      INFO(logger_, "ThreadId:" << ThreadId::Get() << ",try to resume coro id:"
    //        << idx << ",yield times:" << yield_times);
    //    }
    //  }
    //}
  }
}

//挂起当前协程，并将收到消息的协程激活，下次优先切入
bool CoroutineSchedule::CoroutineYield(CoroContext* coro){
  /*if (coro_id < 0 || coro_id >= CoroutineSize()) {
    return false;
  }

  if (GetRunningId() == coro_id) {
    //先返回了，马上又收到了timeout么?
    WARN(logger_, "may be timeout,running id:"<< GetRunningId());
    return false;
  }

  //说明要切换协程，切换回来之后可能是收到了客户端的消息或者超时
  active_coro_list_.emplace(coro_id);
  //if (!available_coro_list_.insert(coro_id).second) {
  //  INFO(logger_, "repeated resume coro id:" << coro_id);
  //  //return false;
  //}*/
  if (GetCurrentCoroutine() != nullptr) {
    //当前是在协程中，切出去
    TRACE(logger_, "cur coro ptr:" << GetCurrentCoroutine() << ",change to coro ptr:" << coro);
    CoroutineYield();
    return true;
  } else{
    WARN(logger_, "warn CoroutineYieldToActive coro ptr:" << coro);
    return false;
  }
}

/*bool CoroutineSchedule::AfterYieldToAvailable(int coro_id) {
  //yield_time_debug_[coro_id].is_yield = false;
  //yield_time_debug_[coro_id].yield_time = time(nullptr);
  if (!available_coro_list_.insert(coro_id).second) {
    INFO(logger_, "repeated resume coro id:" << coro_id);
    //return false;
  }
  return true;
}*/

//启动协程或者恢复有返回的协程,有消息返回，优先切换消费
/*bool CoroutineSchedule::CoroutineResumeActive() {
  if (GetRunningId() >= 0){
    INFO(logger_, "not to here,cur coro id:" << GetRunningId());
    return false;
  }
  if (active_coro_list_.size()>0){
    int id = active_coro_list_.front();
    TRACE(logger_, "change to coro id:" << id);
    active_coro_list_.pop();
    if (id < 0 || id >= CoroutineSize()) {
      WARN(logger_, "error coro id:" << id);
      return false;
    }
    CoroutineResume(id);
    return true;
  }
  return false;
}*/

bool CoroutineSchedule::CoroutineResume(CoroContext* coro) {
  //业务逻辑都在协程中处理，所以调用该函数一定不在协程里
  //if (GetRunningId() >= 0) {
  //  //如果当前是在协程中，先切出去
  //  INFO(logger_, "cur coro id:" << GetRunningId() << ",change to coro id:" << id);
  //  CoroutineManager::SetCurCoroutineId(-1);
  //  CoroutineYield();
  //}
  /*if (id < 0 || id >= CoroutineSize()) {
    WARN(logger_,"invalid id:"<<id);
    return;
  }*/

  //CoroutineManager::SetCurCoroutineId(id);
  //想看详细日志可以打开
  //TRACE(logger_, "will be resume coroutine:" << id);
  return coro_impl_.CoroutineResume(coro);
}

/*int CoroutineSchedule::GetRunningId() {
  return coro_impl_.CoroutineRunning(coro_sche_);
}*/

int CoroutineSchedule::CoroutineStatus() {
  return coro_impl_.CoroutineStatus(coro_impl_.GetCurrentCoroutine());
}

// 挂起当前运行的协程
void CoroutineSchedule::CoroutineYield() {
  coro_impl_.CoroutineYield();
}

void CoroutineID::InitMaxIds(int max_coro_id){
  lock_.lock();

  if (max_coro_id <= (int)(all_coro_ids_tmp_.size())){
    lock_.unlock();
    return;
  }

  //因为这个版本的 coroutine id是从0依次递增
  for (int idx = all_coro_ids_tmp_.size(); idx < max_coro_id; idx++) {
    all_coro_ids_tmp_.emplace_back(idx);
  }

  lock_.unlock();
}

}

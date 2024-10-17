#include "coroutine_schedule.h"
#include "coroutine_actor.h"
#include "coroutine_manager.h"
#include "common/thread_id.h"
#include "coroutine/coroutine_impl_c/coroutine_impl_c.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineSchedule);

CoroutineSchedule::~CoroutineSchedule(){
  coro_impl_->Release();
}

void CoroutineSchedule::InitCoroSchedule(
  CoroutineFunc func, void* data, int coroutine_size, int coroutine_type, int stack_size) {
  coro_func_ = func;
  coro_data_ = data;
  switch (coroutine_type)
  {
  case Coroutine::kCoroutineTypeC:
    coro_impl_ = new CoroutineImplc();
    break;
  default:
    ERROR(logger_, "InitCoroSchedule unkown coroutine_type:" << coroutine_type);
    return;
  }

  coro_impl_->CoroutineInit(func, data, this, free_list_, coroutine_size, stack_size);
}

CoroContext* CoroutineSchedule::GetAvailableCoro() {
  // 用来接收消息的协程是暂时不可用的，其他的协程都是可用的
  for (CoroContext* coro : free_list_) {
    return coro;
  }

  // 不够用，创建新协程
  CoroContext* ret = coro_impl_->CoroutineNew(coro_func_, coro_data_, this);
  auto ok = free_list_.insert(ret);
  INFO(logger_, "ThreadId:" << ThreadId::Get() << ",AddNewCoroutine:" << ret << ",ok" << ok.second);
  return ret;
}

CoroContext* CoroutineSchedule::GetCurrentCoroutine() {
  return coro_impl_->GetCurrentCoroutine();
}

// 支持使用InitCoroSchedule中的值，或者新传入的值
void CoroutineSchedule::CoroutineStart(CoroutineFunc func, void* data) {
  CoroutineFunc func1 = func != nullptr ? func : coro_func_;
  void* data1 = data != nullptr ? data : coro_data_;
  CoroContext* coro = coro_impl_->CoroutineNew(func1, data1, this);
  //free_list_.erase(coro);
  coro_impl_->CoroutineResume(coro);
}

void CoroutineSchedule::CoroutineStart(CoroContext* coro, CoroutineFunc func, void* data) {
  coro_impl_->CoroutineStart(coro, func, data);
}

void CoroutineSchedule::ProcessDebug(){
  static thread_local time_t now = time(NULL);
  //static thread_local time_t now_1 = now;
  time_t cur_time = time(NULL);
  if ((cur_time - now) > 60) {
    //60秒一次trace
    INFO(logger_, "ThreadId:" << ThreadId::Get() << ",free list size:" 
      << free_list_.size() << ",receive list size:" << active_coro_list_.size()
      << ",receiving_cnt_:" << receiving_cnt_);
    now = cur_time;
  }
}

bool CoroutineSchedule::ReleaseResource(CoroContext* coro) {
  INFO(logger_, "ThreadId:" << ThreadId::Get() << ",should release when exit coro:" << coro);
  free_list_.erase(coro);
  return true;
}

//挂起当前协程，并将收到消息的协程激活，下次有限切入
bool CoroutineSchedule::CoroutineYieldToActive(CoroContext* coro) {
  if (nullptr == coro) {
    WARN(logger_, "CoroutineYieldToActive coro is nullptr");
    return false;
  }

  if (GetCurrentCoroutine() == coro) {
    //先返回了，马上又收到了timeout么?
    WARN(logger_, "CoroutineYieldToActive may be timeout,running coro:" << coro);
    //return false;
  }

  //说明要切换协程，切换回来之后可能是收到了客户端的消息或者超时
  active_coro_list_.emplace(coro);
  //receiving_cnt_--;
  if (nullptr != GetCurrentCoroutine()) {
    //当前是在协程中，切出去
    TRACE(logger_, "cur coro ptr:" << GetCurrentCoroutine() << ",will change to coro ptr:" << coro);
    CoroutineYield();
    return true;
  } else {
    WARN(logger_, "warn CoroutineYieldToActive coro ptr:" << coro);
    return false;
  }
}

bool CoroutineSchedule::ScCoroutineResume(CoroContext* coro) {
  //free_list_.erase(coro);
  bool ret = coro_impl_->CoroutineResume(coro);
  //free_list_.insert(coro);
  return ret;
}

// 尝试切换到一个已经接收数据就绪的协程
bool CoroutineSchedule::CoroutineResumeActive() {
  if (GetCurrentCoroutine() != nullptr) {
    WARN(logger_, "CoroutineResumeActive not should to here");
    return false;
  }

  if (active_coro_list_.size() > 0) {
    CoroContext* coro = active_coro_list_.front();
    TRACE(logger_, "CoroutineResumeActive change to coro ptr:" << coro);
    active_coro_list_.pop();
    bool resume = ScCoroutineResume(coro);
    if (!resume) {
      WARN(logger_, "ScCoroutineResume faile coro:" << coro);
    }
    //free_list_.insert(coro);
    //receiving_cnt_--;
    return true;
  }
  return false;
}


int CoroutineSchedule::CoroutineStatus() {
  return coro_impl_->CoroutineStatus(coro_impl_->GetCurrentCoroutine());
}

// 挂起当前运行的协程
void CoroutineSchedule::CoroutineYield() {
  //free_list_.insert(GetCurrentCoroutine());
  coro_impl_->CoroutineYield();
}

// 准备接收数据挂起当前运行的协程
void CoroutineSchedule::ReceiveCoroutineYield() {
  CoroContext* cur = coro_impl_->GetCurrentCoroutine();
  // 从free list里面拿出是为了让收消息的协程不要再被调度上了，直到收到消息或者超时之前
  auto it = free_list_.find(cur);
  if (it != free_list_.end()) {
    free_list_.erase(it);
  } else {
    WARN(logger_, "ThreadId:" << ThreadId::Get() << ",ReceiveCoroutineYield free_list_ not found coro:" << cur);
  }

  receiving_cnt_++;
  coro_impl_->CoroutineYield();
  receiving_cnt_--;

  // 协程切回来之后又可以用了
  free_list_.insert(cur);
}

}

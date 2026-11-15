
#include "event/timer/timer.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, Timer)

Timer::Timer(){
  if (!heap_timer_.InitHeap()) {
    ERROR(logger_, "init timer error.");
  }
}

Timer::~Timer() {
}

uint64_t Timer::AddTimer(uint64_t after_ms, TimerData& data){
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  data.time_out_ms = cur_time_ms + after_ms;
  std::lock_guard<std::mutex> guard(lock_);
  data.timer_id = next_id_++;
  size_t sign;
  heap_timer_.Insert(data, sign);
  id_to_sign_[data.timer_id] = sign;
  return data.timer_id;
}

int Timer::DelTimer(uint64_t id){
  std::lock_guard<std::mutex> guard(lock_);
  auto it = id_to_sign_.find(id);
  if (it == id_to_sign_.end()) {
    return -1; // already fired or cancelled
  }
  heap_timer_.Erase(it->second);
  id_to_sign_.erase(it);
  return 0;
}

bool Timer::PopExpired(uint64_t now, TimerData& out){
  std::lock_guard<std::mutex> guard(lock_);
  TimerData* top = heap_timer_.Top();
  if (nullptr == top || now < top->time_out_ms) {
    return false;
  }
  out = *top;
  heap_timer_.Pop();
  id_to_sign_.erase(out.timer_id);
  return true;
}

int Timer::ProcessTimer(){
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData expired;
  while (PopExpired(cur_time_ms, expired)) {
    if (nullptr != expired.time_proc) {
      expired.time_proc->OnTimer(expired.function_data);
    }
  }
  return 0;
}

int Timer::ProcessTimerTest(std::list<size_t>& ids){
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData expired;
  while (PopExpired(cur_time_ms, expired)) {
    ids.emplace_back(expired.timer_id);
    if (nullptr != expired.time_proc) {
      expired.time_proc->OnTimer(expired.function_data);
    }
  }
  return 0;
}

//协程情况下，使用ProcessTimerCoro不太对，开始认为是取到top之后切换，下一个协程还是取到相同的top
//改为取到top之后，先pop，发现还是不太对
//所以后面协程里面的Timer改为了common目录下的TimerMgr
int Timer::ProcessTimerCoro() {
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData expired;
  while (PopExpired(cur_time_ms, expired)) {
    if (nullptr != expired.time_proc) {
      expired.time_proc->OnTimerCoro(expired.function_data, expired.coro_id);
    }
  }
  return 0;
}

void Timer::Clear(){
  std::lock_guard<std::mutex> guard(lock_);
  heap_timer_.Clear();
  id_to_sign_.clear();
}

}

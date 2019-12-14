
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
  size_t time_id;
  heap_timer_.Insert(data, time_id);
  return time_id;
}

int Timer::DelTimer(uint64_t id){
  heap_timer_.Erase(id);
  return 0;
}

int Timer::ProcessTimer(){
  if (0 == heap_timer_.Size()){
    return 0;
  }
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData* event_timeo = heap_timer_.Top();
  while (nullptr != event_timeo
    && cur_time_ms >= event_timeo->time_out_ms 
    && nullptr != event_timeo->time_proc) {
    event_timeo->time_proc->OnTimer(event_timeo->function_data);
    heap_timer_.Pop();
    event_timeo = heap_timer_.Top();
  }
  return 0;
}

int Timer::ProcessTimerTest(std::list<size_t>& ids){
  if (0 == heap_timer_.Size()) {
    return 0;
  }
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData* event_timeo = heap_timer_.Top();
  while (nullptr != event_timeo
    && cur_time_ms >= event_timeo->time_out_ms
    && nullptr != event_timeo->time_proc) {
    event_timeo->time_proc->OnTimer(event_timeo->function_data);
    ids.emplace_back(heap_timer_.Pop());
    event_timeo = heap_timer_.Top();
  }
  return 0;
}

//协程情况下，使用ProcessTimerCoro不太对，开始认为是取到top之后切换，下一个协程还是取到相同的top
//改为取到top之后，先pop，发现还是不太对
//所以后面协程里面的Timer改为了common目录下的TimerMgr
int Timer::ProcessTimerCoro() {
  if (0 == heap_timer_.Size()) {
    return 0;
  }
  uint64_t cur_time_ms = Time::GetCurrentClockTime();
  TimerData* event_timeo = heap_timer_.Top();
  while (nullptr != event_timeo
    && cur_time_ms >= event_timeo->time_out_ms
    && nullptr != event_timeo->time_proc) {
    heap_timer_.Pop();
    event_timeo->time_proc->OnTimerCoro(
      event_timeo->function_data, event_timeo->coro_id);
    event_timeo = heap_timer_.Top();
  }
  return 0;
}

void Timer::Clear(){
  heap_timer_.Clear();
}

}

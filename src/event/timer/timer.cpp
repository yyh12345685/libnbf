
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
  uint64_t cur_time_ms = Time::GetMillisecond();
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
  uint64_t cur_time_ms = Time::GetMillisecond();
  TimerData* event_timeo = heap_timer_.Top();
  while (nullptr != event_timeo
    && cur_time_ms > event_timeo->time_out_ms 
    && nullptr != event_timeo->time_proc) {
    event_timeo->time_proc->OnTimer(event_timeo->function_data);
    heap_timer_.Pop();
    event_timeo = heap_timer_.Top();
  }
  return 0;
}

void Timer::Clear(){
  heap_timer_.Clear();
}

}

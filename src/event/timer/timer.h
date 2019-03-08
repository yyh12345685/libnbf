
#pragma once

#include <stdint.h>
#include "common/logger.h"
#include "common/heap.h"
#include "event/timer/timer_base.h"

namespace bdf {

namespace event {

template <typename T>
struct GetTimeOut {
  int64_t operator()(const T& t) const { return t.time_out_ms; }
};

//template <typename TD= TimerData>
class Timer {
public:

  Timer();
  ~Timer();
  
  uint64_t AddTimer(uint64_t after_ms, TimerData& data);
  int DelTimer(uint64_t id);

  int ProcessTimer();

private:
	LOGGER_CLASS_DECL(logger_);

  common::SimpleHeap<TimerData, GetTimeOut<TimerData> > heap_timer_;

};

}

}

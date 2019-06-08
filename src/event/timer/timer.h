#pragma once

#include <stdint.h>
#include <list>
#include "common/logger.h"
#include "common/heap.h"
#include "event/timer/timer_base.h"

namespace bdf {

template <typename T>
struct GetTimeOut {
  int64_t operator()(const T& tm) const { return tm.time_out_ms; }
};

class Timer {
public:

  Timer();
  ~Timer();
  
  uint64_t AddTimer(uint64_t after_ms, TimerData& data);
  int DelTimer(uint64_t id);

  int ProcessTimer();

  int ProcessTimerTest(std::list<size_t>& ids);

  void Clear();

private:
	LOGGER_CLASS_DECL(logger_);

  SimpleHeap<TimerData, GetTimeOut<TimerData> > heap_timer_;

};

}

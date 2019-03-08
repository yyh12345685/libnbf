#pragma once

#include <stdint.h>

namespace bdf {

namespace event {

class OnTimerBase {
public:
  virtual void OnTimer(void* function_data) = 0;
  virtual ~OnTimerBase() { }
};

typedef struct{
  uint64_t time_out_ms;
  OnTimerBase* time_proc;
  void* function_data;
}TimerData;

}
}

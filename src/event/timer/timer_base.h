#pragma once

#include <stdint.h>

namespace bdf {

class OnTimerBase {
public:
  virtual void OnTimer(void* function_data) = 0;
  virtual void OnTimerCoro(void* function_data,int& coro_id){}
  virtual ~OnTimerBase() { }
};

typedef struct{
  uint64_t time_out_ms;
  OnTimerBase* time_proc;
  void* function_data;
  int coro_id;
}TimerData;

}

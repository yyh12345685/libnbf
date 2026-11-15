#pragma once

#include <stdint.h>
#include <list>
#include <unordered_map>
#include <mutex>
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
  int ProcessTimerCoro();

  void Clear();

private:
	LOGGER_CLASS_DECL(logger_);

  // Pops the top timer before its callback runs, so the callback may freely
  // add or cancel timers (including itself). Takes lock_.
  bool PopExpired(uint64_t now, TimerData& out);

  // Timers are added/cancelled from other threads (acceptor, main thread,
  // service threads) while the owning loop processes them: guard the heap.
  // Callbacks run without the lock.
  std::mutex lock_;

  SimpleHeap<TimerData, GetTimeOut<TimerData> > heap_timer_;
  // Heap node addresses are recycled, so they cannot be handed out as ids:
  // an expired id would cancel whichever timer reused the node.
  std::unordered_map<uint64_t, size_t> id_to_sign_;
  uint64_t next_id_ = 1;

};

}

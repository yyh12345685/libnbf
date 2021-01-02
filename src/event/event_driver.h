
#pragma once

#include <unordered_set>
#include <deque>
#include "event/event_data.h"
#include "event/event_notifier.h"
#include "event/timer/timer.h"

namespace bdf{

class ThreadDataRun;

class EventDriver{
public:
  EventDriver();
  virtual ~EventDriver();
  int Init();
  int Run();
  int Stop();
  int ShutDown();
  int Add(int fd, EventFunctionBase *ezfd,bool lock = false);
  int Del(int fd);
  int Modr(int fd, bool set);
  int Modw(int fd, bool set);
  int Modrw(int fd, bool set);
  int Poll(int timeout/*msecs*/);

  int Wakeup();

  bool GetRun(){ return run_; }

  uint64_t StartTimer(TimerData& data) {
    return timer_.AddTimer(data.time_out_ms, data);
  }

  void CancelTimer(uint64_t timer_id) {
    timer_.DelTimer(timer_id);
  }

  void SetThreadDataRun(ThreadDataRun* thread_data_run){
    thread_data_run_ = thread_data_run;
  }
private:

  int Mod(int fd);
  
  bool run_;
  bool inloop_;
  int epfd_;
  int maxfd_;
  
  uint32_t event_in_;  
  uint32_t event_out_;

  LOGGER_CLASS_DECL(logger_);

  EventData event_data_;

  EventNotifier event_notifier_;

  Timer timer_;
  ThreadDataRun* thread_data_run_;
};

}

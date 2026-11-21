
#pragma once

#include <unordered_set>
#include <deque>
#include <mutex>
#include <atomic>
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
  // events: initial EVENT_READ/EVENT_WRITE interest, registered in the same
  // EPOLL_CTL_ADD. (Adding with no interest and setting it afterwards left a
  // window where EPOLLERR/HUP - always reported - closed the connection on
  // the loop thread while the adding thread still worked on it.)
  int Add(int fd, EventFunctionBase *ezfd,bool lock = false, short events = EVENT_NONE);
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
  
  // Set before the loop thread starts, cleared by Stop() from another
  // thread. Run() must not set it: a Stop() issued before the new thread
  // reached the loop would be overwritten and join() would hang.
  std::atomic<bool> run_;
  bool inloop_;
  int epfd_;
  int maxfd_;
  
  uint32_t event_in_;  
  uint32_t event_out_;

  LOGGER_CLASS_DECL(logger_);

  EventData event_data_;
  // Add/Mod/Del are called from the acceptor and other threads while the
  // loop thread polls: guards event_data_ and maxfd_. Recursive because
  // OnEvent callbacks run on the loop thread and call Del/Modw themselves;
  // the lock is never held across a callback.
  std::recursive_mutex fd_lock_;

  EventNotifier event_notifier_;

  Timer timer_;
  ThreadDataRun* thread_data_run_;
};

}

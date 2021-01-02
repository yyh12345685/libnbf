
#pragma once

#include <thread>
#include "event/event_driver.h"

namespace bdf {

class EventFunctionBase;
class ThreadDataRun;

class EventLoopThread{
public:
  EventLoopThread();
  ~EventLoopThread();
  int Start(ThreadDataRun* thread_data_run=nullptr);
  int Stop();
  void Main();
  static void *Run(void *arg);

  int Add(int fd, EventFunctionBase *ezfd, bool lock = false);
  int Del(int fd);
  int Modr(int fd, bool set);
  int Modw(int fd, bool set);
  int Modrw(int fd, bool set);
  int Wakeup();

  uint64_t StartTimer(TimerData& data) {
    return poll_.StartTimer(data);
  }

  void CancelTimer(uint64_t timer_id) {
    poll_.CancelTimer(timer_id);
  }

private:
  EventDriver poll_;
  bool is_run_;

  std::thread* thread_;

  LOGGER_CLASS_DECL(logger);
};

}

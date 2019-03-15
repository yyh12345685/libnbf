
#pragma once

#include <thread>
#include "event/event_driver.h"

namespace bdf {

class EventFunctionBase;

class EventLoopThread{
public:
  EventLoopThread();
  ~EventLoopThread();
  int Start();
  int Stop();
  void Main();
  static void *Run(void *arg);

  int Add(int fd, EventFunctionBase *ezfd, bool lock = false);
  int Del(int fd);
  int Modr(int fd, bool set);
  int Modw(int fd, bool set);
  int Modrw(int fd, bool set);
  int Wakeup();

private:
  EventDriver poll_;
  bool is_run_;

  std::thread* thread_;

  LOGGER_CLASS_DECL(logger);
};

}

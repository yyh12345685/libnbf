
#pragma once

#include <thread>
#include "event/event_driver.h"

namespace bdf {

class EventLoopThread{
public:
  EventLoopThread();
  ~EventLoopThread();
  int Start();
  int Stop();
  void Main();
  EventDriver *GetPoll();
  static void *Run(void *arg);

private:
  EventDriver poll_;
  bool is_run_;

  std::thread* thread_;

  LOGGER_CLASS_DECL(logger);
};

}

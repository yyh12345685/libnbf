
#pragma once

#include <thread>
#include "event/event_driver.h"

namespace bdf {

namespace event {

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
};

}
}
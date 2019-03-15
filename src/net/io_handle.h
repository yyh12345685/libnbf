#pragma once

#include <stdint.h>
#include "event/timer/timer.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"
#include "handle.h"

namespace bdf{

class EventMessage;
struct HandleData;

class IoHandler:public Handler{
public:
  IoHandler(){}
  virtual ~IoHandler() {}
    
  static void Run(void* handle,HandleData* data);
  void Handle(EventMessage* message);

  static IoHandler* GetCurrentIOHandler() {
    return io_handler_;
  }

  uint64_t StartTimer(TimerData& data) {
    return timer_.AddTimer(data.time_out_ms, data);
  }

  void CancelTimer(uint64_t timer_id) {
    timer_.DelTimer(timer_id);
  }

private:

  void HandleIoMessageEvent(EventMessage* message);
  void HandleIoActiveCloseEvent(EventMessage* message);

  static thread_local IoHandler* io_handler_;

  Timer timer_;

  LOGGER_CLASS_DECL(logger);
};

}

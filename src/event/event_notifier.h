
#pragma once

#include "event/event_data.h"
#include "common/logger.h"

namespace bdf {

class WakeUpFd : public EventFunctionBase {
public:
  virtual void OnEvent(EventDriver *poll, int fd, short event);
private:
  LOGGER_CLASS_DECL(logger);
};

class EventNotifier {
public:

  EventNotifier(EventDriver* event_driver):
    event_driver_(event_driver){
    wake_ = nullptr;
    wake_fd_[0] = wake_fd_[1] = -1;
  }

  ~EventNotifier();

  int InitWakeup();
  int Wakeup();

private:
  int wake_fd_[2];
  WakeUpFd *wake_;

  EventDriver* event_driver_;

  LOGGER_CLASS_DECL(logger);
};

}

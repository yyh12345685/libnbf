
#pragma once

#include "event/event_data.h"
#include "common/logger.h"

namespace bdf {

class WakeUpFd : public EventFunctionBase {
public:
  virtual void OnEvent(EventDriver *poll, int fd, short event);
private:
  LOGGER_CLASS_DECL(logger_);
};

class EventNotifier {
public:

  EventNotifier(EventDriver* event_driver):
    event_fd_(-1),
    event_thread_id_(0),
    wake_(nullptr),
    event_driver_(event_driver){
  }

  ~EventNotifier();

  int InitWakeup();
  int Wakeup();

  void SetEventThreadId(uint32_t thread_id) {
    event_thread_id_ = thread_id;
  }
private:

  int event_fd_;
  uint32_t event_thread_id_;

  WakeUpFd *wake_;

  EventDriver* event_driver_;

  LOGGER_CLASS_DECL(logger_);
};

}

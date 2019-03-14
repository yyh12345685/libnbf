#pragma once

#include "common/spin_lock.h"
#include "common/logger.h"

namespace bdf {

enum{
  EVENT_NONE = 0x00,
  EVENT_READ = 0x01,
  EVENT_WRITE = 0x02,
  EVENT_ERROR = 0x04,
  EVENT_CONNECT_CLOSED = 0x08,
};

class EventDriver;

class EventFunctionBase{
public:
  virtual void OnEvent(EventDriver *poll, int fd, short event) = 0;
  virtual ~EventFunctionBase() { };
};

struct FdEvent{
  int fd_;
  EventFunctionBase *ezfd_;
  short event_;
  bool closed_;
};

class EventData{
public:
  bool InitEventData();
  bool ReInitEventData(int& fd);

  bool ReInitClosed(int& fd);
  void RemoveClosed();
  ~EventData();

  FdEvent **fd2data_;
  int fd2data_size_;

  int closed_size_;
  int closed_count_;
  FdEvent **closed_;

  SpinLock lock_;

private:
  LOGGER_CLASS_DECL(logger_);
};

}

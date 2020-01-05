
#pragma once

#include <vector>
#include "event/event_loop_thread.h"
#include "common/common.h"

namespace bdf {

class NetThreadManager;
class IoThreadDataRun;
class EventFunctionBase;
class EventMessage;

class IoThreads {

public:
  IoThreads();
  bool Init(int io_thread_count);
  bool Start();

  void Stop();
  virtual ~IoThreads();

  int AddModrw(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int AddModr(
    EventFunctionBase *ezfd,int fd,  bool set,bool lock,int& register_thread_id);
    
  int Del(EventFunctionBase* ezfd, int fd);

  void PutMessageToHandle(EventMessage* msg);

  uint64_t StartTimer(TimerData& data,int io_thread_id);
  void CancelTimer(uint64_t timer_id,int io_thread_id);
private:

  LOGGER_CLASS_DECL(logger_);
private:
  std::vector<EventLoopThread*> event_threads_;
  std::vector<IoThreadDataRun*>io_thread_data_run_;

  DISALLOW_COPY_AND_ASSIGN(IoThreads)
};

}

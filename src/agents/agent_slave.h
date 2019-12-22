
#pragma once

#include <vector>
#include "event/event_loop_thread.h"
#include "common/common.h"
#include "slave_service_data_run.h"

namespace bdf {

class Agents;
class EventFunctionBase;
class EventMessage;

class AgentSlave {

public:
  AgentSlave();
  bool Init(int slave_thread_count);
  bool Start();

  void Stop();
  virtual ~AgentSlave();

  int AddModrw(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int AddModr(
    EventFunctionBase *ezfd,int fd,  bool set,bool lock,int& register_thread_id);
    
  int Del(EventFunctionBase* ezfd, int fd);

  void PutMessageToHandle(EventMessage* msg);

  uint64_t StartTimer(TimerData& data,int slave_thread_id);
  void CancelTimer(uint64_t timer_id,int slave_thread_id);
private:

  LOGGER_CLASS_DECL(logger_);
private:
  std::vector<EventLoopThread*> slave_event_threads_;
  std::vector<SlaveServiceDataRun*>slaves_service_data_run_;

  DISALLOW_COPY_AND_ASSIGN(AgentSlave)
};

}

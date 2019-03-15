
#pragma once

#include <vector>
#include "event/event_loop_thread.h"
#include "common/common.h"

namespace bdf {

class Agents;
class EventFunctionBase;

class AgentSlave {
 public: 
  typedef AgentSlave Self;

public:
  AgentSlave();
  bool Init(int slave_thread_count);
  bool Start();

  void Stop();
  virtual ~AgentSlave();

  int AddModr(EventFunctionBase *ezfd,int fd,  bool set,bool lock = false);
  int Del(EventFunctionBase* ezfd, int fd);

private:

  LOGGER_CLASS_DECL(logger_);
private:
  std::vector<EventLoopThread*> slave_event_threads_;

  DISALLOW_COPY_AND_ASSIGN(AgentSlave)
};

}

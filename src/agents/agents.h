
#pragma once

#include <vector>
#include <memory>
#include "common/common.h"
#include "common/logger.h"
#include "event/timer/timer_base.h"

namespace bdf {

struct ServiceConfig;
class AgentMaster;
class AgentSlave;
class EventFunctionBase;
class EventMessage;

class Agents {
public:
  Agents(const ServiceConfig* conf);

  bool Init();
  bool Start();

  void Stop();
  virtual ~Agents();

  AgentMaster* GetMaster()const{ return agent_master_; }
  AgentSlave* GetSlave()const{ return agent_slaves_; }

  int AddModrw(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int AddModr(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int Del(EventFunctionBase* ezfd, int fd);

  void PutMessageToHandle(EventMessage* msg);
  
  uint64_t StartTimer(TimerData& data,int slave_thread_id);
  void CancelTimer(uint64_t timer_id,int slave_thread_id);
private:
  Agents();

  LOGGER_CLASS_DECL(logger_);

  const ServiceConfig* conf_;
  AgentMaster* agent_master_;
  AgentSlave* agent_slaves_; 

  DISALLOW_COPY_AND_ASSIGN(Agents)
};

}


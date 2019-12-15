
#pragma once

#include <vector>
#include <memory>
#include "common/common.h"
#include "common/logger.h"

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

  int AddModrw(EventFunctionBase *ezfd, int fd, bool set, bool lock = false);
  int AddModr(EventFunctionBase *ezfd, int fd, bool set, bool lock = false);
  int Del(EventFunctionBase* ezfd, int fd);
  void PutMessageToHandle(EventMessage* msg);
private:
  Agents();

  LOGGER_CLASS_DECL(logger_);

  const ServiceConfig* conf_;
  AgentMaster* agent_master_;
  AgentSlave* agent_slaves_; 

  DISALLOW_COPY_AND_ASSIGN(Agents)
};

}


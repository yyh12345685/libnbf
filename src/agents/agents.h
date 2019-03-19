
#pragma once

#include <vector>
#include <memory>
#include "common/common.h"
#include "common/logger.h"

namespace bdf {

struct IoServiceConfig;
class IoService;
class AgentMaster;
class AgentSlave;
class EventFunctionBase;

class Agents {
public:
  Agents(const IoServiceConfig* conf);

  bool Init();
  bool Start();

  void Stop();
  virtual ~Agents();

  AgentMaster* GetMaster()const{ return agent_master_; }
  AgentSlave* GetSlave()const{ return agent_slaves_; }

  int AddModrw(EventFunctionBase *ezfd, int fd, bool set, bool lock = false);
  int AddModr(EventFunctionBase *ezfd, int fd, bool set, bool lock = false);
  int Del(EventFunctionBase* ezfd, int fd);

private:
  Agents();

  LOGGER_CLASS_DECL(logger_);

  const IoServiceConfig* conf_;
  AgentMaster* agent_master_;
  AgentSlave* agent_slaves_; 

  DISALLOW_COPY_AND_ASSIGN(Agents)
};

}


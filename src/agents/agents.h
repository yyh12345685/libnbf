
#pragma once

#include <vector>
#include <memory>

#include "common/logger.h"

namespace bdf {

struct AppCofing;
class AgentMaster;
class AgentSlave;
class HandlerMessageFactoryBase;
class HandlerMessageBase;

class Agents {
public:
  Agents(const AppCofing* conf);

  bool Init(const HandlerMessageFactoryBase* factory);
  bool Start();

  void Stop();
  virtual ~Agents();

  AgentMaster* GetMaster()const{ return agent_master_; }
  AgentSlave* GetSlave()const{ return agent_slaves_; }

private:
  Agents(const Agents&);
  Agents();
  Agents& operator=(const Agents&);

  LOGGER_CLASS_DECL(logger);

  const AppCofing* conf_;
  AgentMaster* agent_master_;
  AgentSlave* agent_slaves_; 

  //std::vector<std::tr1::shared_ptr<HandlerMessageBase> >handle_list_;
};

}


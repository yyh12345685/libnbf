
#pragma once

#include <unordered_map>
#include "event/event_loop_thread.h"
#include "event/event_data.h"
#include "common/common.h"

namespace bdf {

struct ServiceConfig;
class Agents;
class ServerConnectDelayReleaseMgr;
class ServerConnect;

class AgentMaster:public EventFunctionBase {
public:
  AgentMaster();
  virtual ~AgentMaster();

  bool Init(const ServiceConfig* confs,const Agents* agents);
  bool Start();
  void Stop();

  virtual void OnEvent(EventDriver *poll, int fd, short event);

  bool AreaseReleasedConnect(ServerConnect* con);
private:

  const ServiceConfig* conf_;
  const Agents* agents_;

  EventLoopThread master_event_thread_;

  std::unordered_map<int,std::pair<int,int> > listened_fd_list_;

  ServerConnectDelayReleaseMgr* release_mgr_;

  int idle_fd_;
  
  LOGGER_CLASS_DECL(logger_);

  DISALLOW_COPY_AND_ASSIGN(AgentMaster)
};

}


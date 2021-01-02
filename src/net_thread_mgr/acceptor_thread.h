
#pragma once

#include <unordered_map>
#include "event/event_loop_thread.h"
#include "event/event_data.h"
#include "common/common.h"

namespace bdf {

struct ServiceConfig;
class NetThreadManager;
class ServerConnectDelayReleaseMgr;
class ServerConnect;

class AcceptorThread:public EventFunctionBase {
public:
  AcceptorThread();
  virtual ~AcceptorThread();

  bool Init(const ServiceConfig* confs,const NetThreadManager* net_thread_mgr);
  bool Start();
  void Stop();

  virtual void OnEvent(EventDriver *poll, int fd, short event);

  bool AreaseReleasedConnect(ServerConnect* con);
protected:
  void AcceptClient(EventDriver *poll,int fd,std::pair<int,int>& server_cate_port);
  void AcceptClientV1(EventDriver *poll,int fd,std::pair<int,int>& server_cate_port);
private:

  const ServiceConfig* conf_;
  const NetThreadManager* net_thread_mgr_;

  EventLoopThread event_thread_;

  std::unordered_map<int,std::pair<int,int> > listened_fd_list_;

  ServerConnectDelayReleaseMgr* release_mgr_;

  int idle_fd_;
  
  LOGGER_CLASS_DECL(logger_);

  DISALLOW_COPY_AND_ASSIGN(AcceptorThread)
};

}


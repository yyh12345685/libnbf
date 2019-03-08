
#pragma once

#include <unordered_map>
#include "event/event_loop_thread.h"
#include "event/event_data.h"

namespace bdf {
namespace agents {

class Agents;
class AppCofing;
class Connecting;

struct ListenInfo{
  int cate;
  std::string addr;
};

class AgentMaster:public event::EventFunctionBase {
public:
  AgentMaster();
  bool Init(const AppCofing* confs, const Agents* agents);
  bool Start();

  void Stop();
  virtual ~AgentMaster();

  virtual void OnEvent(event::EventDriver *poll, int fd, short event);
private:
  AgentMaster(const AgentMaster&);
  AgentMaster& operator=(const AgentMaster&);

  const AppCofing* conf_;
  const Agents* agents_;

  event::EventLoopThread master_event_thread_;

  std::unordered_map<int, ListenInfo> listened_list_;

  LOGGER_CLASS_DECL(logger_);
};

}

}


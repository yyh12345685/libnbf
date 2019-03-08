
#pragma once

#include "event/event_loop_thread.h"

namespace bdf {
namespace agents {

class Agents;

class AgentSlave {
 public: 
  typedef AgentSlave Self;

public:
  AgentSlave();
  bool Init(int slave_thread_count, const Agents* agents);
  bool Start();

  void Stop();
  virtual ~AgentSlave();
  
private:
  AgentSlave(const AgentSlave&);
  AgentSlave& operator=(const AgentSlave&);
private:
  std::vector<event::EventLoopThread> slave_event_threads_;
  const Agents* agents_;
};

}

}


#include "agents/agent_slave.h"
#include "agents/agents.h"

namespace bdf {
namespace agents {

AgentSlave::AgentSlave():
  agents_(NULL){}

bool AgentSlave::Init(int slave_thread_count, const Agents* agents)
{
  agents_ = agents;
  slave_event_threads_.resize(slave_thread_count);
  return true;
}

bool AgentSlave::Start() 
{
  for (auto& thread: slave_event_threads_){
    thread.Start();
  }
  return true;
}

void AgentSlave::Stop() 
{
  for (auto& thread : slave_event_threads_) {
    thread.Stop();
  }
}

AgentSlave::~AgentSlave() 
{
  Stop();
}

}

}

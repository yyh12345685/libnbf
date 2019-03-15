
#include "agents/agent_slave.h"
#include "agents/agents.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, AgentSlave)

AgentSlave::AgentSlave(){

}

AgentSlave::~AgentSlave() {
  Stop();
  for (auto& thread : slave_event_threads_) {
    delete thread;
  }
}

bool AgentSlave::Init(int slave_thread_count){
  for (int ix = 0; ix < slave_thread_count;ix++) {
    EventLoopThread* evt = new EventLoopThread;
    slave_event_threads_.push_back(evt);
  }
  return true;
}

bool AgentSlave::Start() {
  for (auto& thread: slave_event_threads_){
    thread->Start();
  }
  return true;
}

void AgentSlave::Stop() {
  TRACE(logger_, "AgentSlave::Stop.");
  for (auto& thread : slave_event_threads_) {
    thread->Stop();
  }
}

int AgentSlave::AddModr(EventFunctionBase* ezfd,int fd,  bool set, bool lock){
  uint64_t idx = (uint64_t)((void*)ezfd) % slave_event_threads_.size();
  if (0!= slave_event_threads_[idx]->Add(fd, ezfd, lock)){
    WARN(logger_, "slave_event_threads_[idx].Add failed.");
    return -1;
  }

  if (0 != slave_event_threads_[idx]->Modr(fd,set)) {
    WARN(logger_, "slave_event_threads_[idx].Modr failed.");
    return -2;
  }

  return slave_event_threads_[idx]->Wakeup();
}

int AgentSlave::Del(EventFunctionBase* ezfd, int fd){
  uint64_t idx = (uint64_t)((void*)ezfd) % slave_event_threads_.size();
  if (0 != slave_event_threads_[idx]->Del(fd)) {
    WARN(logger_, "slave_event_threads_[idx].Modr failed.");
    return -1;
  }
  return 0;
}

}

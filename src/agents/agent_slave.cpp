
#include "agents/agent_slave.h"
#include "agents/agents.h"
#include "monitor/mem_profile.h"
#include "handle_data.h"
#include "message_base.h"
#include "net/connect.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, AgentSlave)

AgentSlave::AgentSlave(){

}

AgentSlave::~AgentSlave() {
  //Stop();
  for (auto& thread : slave_event_threads_) {
    BDF_DELETE(thread);
  }

  for (auto& ssdr : slaves_service_data_run_) {
    BDF_DELETE(ssdr);
  }
}

bool AgentSlave::Init(int slave_thread_count){
  for (int ix = 0; ix < slave_thread_count;ix++) {
    EventLoopThread* evt = BDF_NEW(EventLoopThread);
    slave_event_threads_.push_back(evt);
    HandleData* hd = new HandleData;
    SlaveServiceDataRun* ssdr = BDF_NEW(SlaveServiceDataRun,hd);
    slaves_service_data_run_.push_back(ssdr);
  }

  return true;
}

bool AgentSlave::Start() {
  for (size_t idx =0; idx<slave_event_threads_.size();idx++){
    slave_event_threads_[idx]->Start(slaves_service_data_run_[idx]);
  }
  return true;
}

void AgentSlave::Stop() {
  INFO(logger_, "AgentSlave::Stop.");
  for (auto& thread : slave_event_threads_) {
    thread->Stop();
  }
}

int AgentSlave::AddModrw(EventFunctionBase *ezfd, int fd, bool set, bool lock){
  uint64_t idx = (uint64_t)((void*)ezfd) % slave_event_threads_.size();
  if (0 != slave_event_threads_[idx]->Add(fd, ezfd, lock)) {
    WARN(logger_, "slave_event_threads_[idx].Add failed.");
    return -1;
  }

  if (0 != slave_event_threads_[idx]->Modr(fd, set)) {
    WARN(logger_, "slave_event_threads_[idx].Modr failed.");
    return -2;
  }

  if (0 != slave_event_threads_[idx]->Modw(fd, set)) {
    WARN(logger_, "slave_event_threads_[idx].Modw failed.");
    return -3;
  }

  return slave_event_threads_[idx]->Wakeup();
}

int AgentSlave::AddModr(EventFunctionBase* ezfd,int fd,  bool set, bool lock){
  uint64_t idx = (uint64_t)((void*)ezfd) % slave_event_threads_.size();
  if (0!= slave_event_threads_[idx]->Add(fd, ezfd, lock)){
    WARN(logger_, "slave_event_threads_[idx].Add failed,fd:"<<fd);
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
    TRACE(logger_, "slave_event_threads_[idx].Modr failed or fd:"<<fd<<" Already del.");
    return -1;
  }
  return 0;
}

void AgentSlave::PutMessageToHandle(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_io(0);
  uint32_t id = 0;
  if (0!= msg->descriptor_id){
    Connecting* con = (Connecting*)((void*)(msg->descriptor_id));
    //不能用con指针地址取模，回导致线程的队列分布非常不均匀，使用顺序id即可
    id = con->GetConnectId() %slaves_service_data_run_.size();
  } else {
    id = (id_io++) % slaves_service_data_run_.size();
  }
  slaves_service_data_run_[id]->PutMessage(msg);
}

}

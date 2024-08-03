
#include "net_thread_mgr/io_threads.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_thread_data_run.h"
#include "monitor/mem_profile.h"
#include "handle_data.h"
#include "message_base.h"
#include "net/connect.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, IoThreads)

IoThreads::IoThreads(){
}

IoThreads::~IoThreads() {
  //Stop();
  for (auto& thread : event_threads_) {
    BDF_DELETE(thread);
  }

  for (auto& ssdr : io_thread_data_run_) {
    BDF_DELETE(ssdr);
  }
}

bool IoThreads::Init(int io_thread_count){
  for (int ix = 0; ix < io_thread_count;ix++) {
    EventLoopThread* evt = BDF_NEW(EventLoopThread);
    event_threads_.push_back(evt);
    HandleData* hd = new HandleData;
    IoThreadDataRun* hdr = BDF_NEW(IoThreadDataRun,hd);
    io_thread_data_run_.push_back(hdr);
  }

  return true;
}

bool IoThreads::Start() {
  for (size_t idx =0; idx<event_threads_.size();idx++){
    event_threads_[idx]->Start(io_thread_data_run_[idx]);
  }
  return true;
}

void IoThreads::Stop() {
  INFO(logger_, "IoThreads::Stop.");
  for (auto& thread : event_threads_) {
    thread->Stop();
  }
}

int IoThreads::AddModrw(
  EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id){
  int idx = (uint64_t)((void*)ezfd) % event_threads_.size();
  register_thread_id = idx;
  if (0 != event_threads_[idx]->Add(fd, ezfd, lock)) {
    WARN(logger_, "event_threads_[idx].Add failed.");
    return -1;
  }

  if (0 != event_threads_[idx]->Modr(fd, set)) {
    WARN(logger_, "event_threads_[idx].Modr failed.");
    return -2;
  }

  if (0 != event_threads_[idx]->Modw(fd, set)) {
    WARN(logger_, "event_threads_[idx].Modw failed.");
    return -3;
  }

  return event_threads_[idx]->Wakeup();
}

int IoThreads::AddModr(
  EventFunctionBase* ezfd,int fd,  bool set, bool lock,int& register_thread_id){
  int idx = (uint64_t)((void*)ezfd) % event_threads_.size();
  register_thread_id= idx;
  if (0!= event_threads_[idx]->Add(fd, ezfd, lock)){
    WARN(logger_, "event_threads_[idx].Add failed,fd:"<<fd);
    return -1;
  }

  if (0 != event_threads_[idx]->Modr(fd,set)) {
    WARN(logger_, "event_threads_[idx].Modr failed.");
    return -2;
  }

  return event_threads_[idx]->Wakeup();
}

int IoThreads::Del(EventFunctionBase* ezfd, int fd){
  uint64_t idx = (uint64_t)((void*)ezfd) % event_threads_.size();
  if (0 != event_threads_[idx]->Del(fd)) {
    TRACE(logger_, "event_threads_[idx].Modr failed or fd:"<<fd<<" Already del.");
    return -1;
  }
  return 0;
}

void IoThreads::PutMessageToHandle(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_io(0);
  uint32_t id = 0;
  if (0!= msg->descriptor_id){
    //Connecting* con = (Connecting*)((void*)(msg->descriptor_id));
    //不能用con指针地址取模，回导致线程的队列分布非常不均匀，使用顺序id即可
    //id = con->GetConnectId() %io_thread_data_run_.size();
    //为了无锁，只能取摸,仔细看代码会发现id和GetRegisterThreadId()一致
    id = msg->descriptor_id % io_thread_data_run_.size();
  } else {
    id = (id_io++) % io_thread_data_run_.size();
  }
  io_thread_data_run_[id]->PutMessage(msg);
}

uint64_t IoThreads::StartTimer(TimerData& data,int io_thread_id){
  if(io_thread_id < 0 || 
    io_thread_id >= (int)(event_threads_.size())){
    WARN(logger_, "start,invalid io thread id:"<<io_thread_id);
    return 0;
  }
  return event_threads_[io_thread_id]->StartTimer(data);
}

void IoThreads::CancelTimer(uint64_t timer_id,int io_thread_id){
if(io_thread_id < 0 || 
    io_thread_id >= (int)(event_threads_.size())){
    TRACE(logger_, "cancel, invalid io thread id:"<<io_thread_id);
    return ;
  }
  return event_threads_[io_thread_id]->CancelTimer(timer_id);
}

}

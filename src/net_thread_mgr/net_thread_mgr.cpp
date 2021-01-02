
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/acceptor_thread.h"
#include "net_thread_mgr/io_threads.h"
#include "app/config_info.h"
#include "monitor/mem_profile.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, NetThreadManager)

NetThreadManager::NetThreadManager(const ServiceConfig* conf) :
  conf_(conf),
  acceptor_thread_(nullptr),
  io_threads_(nullptr)
	{}

bool NetThreadManager::Init(){

  io_threads_ = BDF_NEW(IoThreads);
	if(!io_threads_->Init(conf_->io_thread_count)){
		return false;
	}

  acceptor_thread_ = BDF_NEW(AcceptorThread);
  return acceptor_thread_->Init(conf_,this);
}

bool NetThreadManager::Start() {

  bool ret = acceptor_thread_->Start();
  if (!ret){
    return false;
  }

  io_threads_->Start();
  
  return true;
}

void NetThreadManager::Stop(){
  INFO(logger_, "NetThreadManager::Stop.");
	if(nullptr != acceptor_thread_)
    acceptor_thread_->Stop();
  INFO(logger_, "acceptor_thread_ Stop ok.");
	if(nullptr != io_threads_)
    io_threads_->Stop();
  INFO(logger_, "io_threads_ Stop ok.");
}

int NetThreadManager::AddModrw(
  EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id){
  return io_threads_->AddModrw(ezfd, fd, set, lock,register_thread_id);
}

int NetThreadManager::AddModr(
  EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id){
  return io_threads_->AddModr(ezfd, fd, set, lock,register_thread_id);
}

int NetThreadManager::Del(EventFunctionBase* ezfd, int fd){
  return io_threads_->Del(ezfd, fd);
}

void NetThreadManager::ReleaseServerCon(ServerConnect* svr_con){
  acceptor_thread_->AreaseReleasedConnect(svr_con);
}

void NetThreadManager::PutMessageToHandle(EventMessage* msg){
  io_threads_->PutMessageToHandle(msg);
}

uint64_t NetThreadManager::StartTimer(TimerData& data,int io_thread_id){
  return io_threads_->StartTimer(data,io_thread_id);
}

void NetThreadManager::CancelTimer(uint64_t timer_id,int io_thread_id){
  return io_threads_->CancelTimer(timer_id,io_thread_id);
}

NetThreadManager::~NetThreadManager() {
  //Stop();
	
	if(nullptr != acceptor_thread_)
    BDF_DELETE(acceptor_thread_);
	if(nullptr != io_threads_)
    BDF_DELETE(io_threads_);
}

}

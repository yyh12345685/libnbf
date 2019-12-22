
#include "agents/agents.h"
#include "agents/agent_master.h"
#include "agents/agent_slave.h"
#include "app/config_info.h"
#include "monitor/mem_profile.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, Agents)

Agents::Agents(const ServiceConfig* conf) :
  conf_(conf),
  agent_master_(nullptr),
  agent_slaves_(nullptr)
	{}

bool Agents::Init(){

  agent_slaves_ = BDF_NEW(AgentSlave);
	if(!agent_slaves_->Init(conf_->slave_thread_count)){
		return false;
	}

  agent_master_ = BDF_NEW(AgentMaster);
  return agent_master_->Init(conf_,this);
}

bool Agents::Start() {

  bool ret = agent_master_->Start();
  if (!ret){
    return false;
  }

  agent_slaves_->Start();
  
  return true;
}

void Agents::Stop(){
  INFO(logger_, "Agents::Stop.");
	if(nullptr != agent_master_)
    agent_master_->Stop();
  INFO(logger_, "master Stop ok.");
	if(nullptr != agent_slaves_)
    agent_slaves_->Stop();
  INFO(logger_, "slave Stop ok.");
}

int Agents::AddModrw(
  EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id){
  return agent_slaves_->AddModrw(ezfd, fd, set, lock,register_thread_id);
}

int Agents::AddModr(
  EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id){
  return agent_slaves_->AddModr(ezfd, fd, set, lock,register_thread_id);
}

int Agents::Del(EventFunctionBase* ezfd, int fd){
  return agent_slaves_->Del(ezfd, fd);
}

void Agents::PutMessageToHandle(EventMessage* msg){
  agent_slaves_->PutMessageToHandle(msg);
}

uint64_t Agents::StartTimer(TimerData& data,int slave_thread_id){
  return agent_slaves_->StartTimer(data,slave_thread_id);
}

void Agents::CancelTimer(uint64_t timer_id,int slave_thread_id){
  return agent_slaves_->CancelTimer(timer_id,slave_thread_id);
}

Agents::~Agents() {
  //Stop();
	
	if(nullptr != agent_master_)
    BDF_DELETE(agent_master_);
	if(nullptr != agent_slaves_)
    BDF_DELETE(agent_slaves_);
}

}

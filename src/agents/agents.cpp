

#include "agents/agents.h"
#include "agents/agent_master.h"
#include "agents/agent_slave.h"
#include "monitor/mem_profile.h"
//#include "app.h"
//#include "handler.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, Agents)

Agents::Agents(const AppCofing* conf) :
  conf_(conf),
  agent_master_(NULL),
  agent_slaves_(NULL)
	{}

bool Agents::Init(const HandlerMessageFactoryBase*factory){
  //for (int idx = 0; idx < conf_->slave_thread_count_; idx++){
  //  HandlerMessageBase* handle = factory->NewHandler();
  //  handle->Init();

		//std::tr1::shared_ptr<HandlerMessageBase> handle_ptr(handle);
  //  handle_list_.push_back(handle_ptr);
  //}

  agent_slaves_ = BDF_NEW(AgentSlave);

	if(!agent_slaves_->Init(conf_->slave_thread_count_,this)){
		return false;
	}

  agent_master_ = BDF_NEW(AgentMaster);
  return agent_master_->Init(conf_,this);
}

bool Agents::Start() {
  for (const auto& handle : handle_list_){
    handle->Start();
  }

  bool ret = agent_master_->Start();
  if (!ret){
    return false;
  }

  agent_slaves_->Start();
  
  return true;
}

void Agents::Stop(){
	if(NULL != agent_master_)
    agent_master_->Stop();
	if(NULL != agent_slaves_)
    agent_slaves_->Stop();
  for (const auto& handle:handle_list_){
    handle->Finish();
  }
}

Agents::~Agents() {
  Stop();
	
	if(NULL != agent_master_)
    BDF_DELETE(agent_master_);
	if(NULL != agent_slaves_)
    BDF_DELETE(agent_slaves_);
}

}

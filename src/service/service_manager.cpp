
#include <functional>
#include <atomic>
#include "service/service_manager.h"
#include "agents/agents.h"
#include "service/service_handle.h"
#include "net/connect.h"
#include "message_base.h"
#include "monitor/matrix.h"
#include "monitor/matrix_stat_map.h"
#include "monitor/mem_profile.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ServiceManager);

int ServiceManager::Init(const ServiceConfig& config){
  agents_ = BDF_NEW(Agents,&config);
  if (!agents_->Init()){
    BDF_DELETE (agents_);
    return -1;
  }

  monitor::GlobalMatrix::Init(config.monitor_file_name,
    config.monitor_token_bucket,
    config.monitor_queue_bucket,
    config.monitor_queue_size);

  service_config_ = config;
  return 0;
}

int ServiceManager::Start(ServiceHandler* handle){
  if (!agents_->Start()){
    delete handle;
    return -1;
  }

  for (int cn = 0; cn < service_config_.service_handle_thread_count; cn++) {
    HandleData* hd = new HandleData;
    hd->handle_ = handle->Clone();
    hd->is_run = true;
    hd->handle_->SetHandlerId(cn);
    handle_thread_.service_handle_data_.push_back(hd);
    std::thread* thread = new std::thread(std::bind(&Handler::Run, hd->handle_, hd));
    handle_thread_.service_handle_thread_.push_back(thread);
  }

  delete handle;
  return 0;
}

int ServiceManager::ThreadWait(){
  INFO(logger_, "ServiceManager::ThreadWait start.");
  for (int cn = 0; cn < service_config_.service_handle_thread_count; cn++){
    handle_thread_.service_handle_thread_[cn]->join();
  }

  INFO(logger_, "ServiceManager::ThreadWait end.");
  return 0;
}

int ServiceManager::ThreadStop(){
  INFO(logger_, "ServiceManager::ThreadStop.");
  for (int cn = 0; cn < service_config_.service_handle_thread_count; cn++){
    handle_thread_.service_handle_data_[cn]->is_run = false;
  }

  INFO(logger_, "ServiceManager::Set Stope ok.");
  return 0;
}

int ServiceManager::Stop(){
  agents_->Stop();

  ThreadStop();
  ThreadWait();

  if (agents_ != nullptr) {
    BDF_DELETE(agents_);
  }

  monitor::GlobalMatrix::Destroy();
  return 0;
}

int ServiceManager::Wait(){
  return ThreadWait();
}

void ServiceManager::SendCloseToSlaveThread(EventMessage* msg){
  msg->type_io_event = MessageType::kIoActiveCloseEvent;
  SendToSlaveThreadInner(msg);
}

void ServiceManager::Reply(EventMessage* message){
  message->direction = EventMessage::kOutgoingResponse;
  SendToSlaveThread(message);
}

void ServiceManager::SendToSlaveThread(EventMessage* msg){
  msg->type_io_event = MessageType::kIoHandleEventMsg;
  SendToSlaveThreadInner(msg);
}

uint32_t ServiceManager::GetServiceHandleId(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_hs(0);
  if (msg->handle_id >=0 && 
    msg->handle_id < (int32_t)(handle_thread_.service_handle_thread_.size())){
    return msg->handle_id;
  }
  return (id_hs++) % handle_thread_.service_handle_data_.size();
}

uint32_t ServiceManager::GetServiceHandleCount() {
  return handle_thread_.service_handle_thread_.size();
}

void ServiceManager::SendToServiceHandle(EventMessage* msg){
  uint32_t id = GetServiceHandleId(msg);
  TRACE(logger_, "handle id:" << msg->handle_id << ",Get id:" << id);
  HandleData* hd = handle_thread_.service_handle_data_.at(id);
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

void ServiceManager::SendTaskToServiceHandle(Task* task){
  static thread_local std::atomic<uint32_t> id_task(0);
  uint32_t tid = (id_task++) % handle_thread_.service_handle_data_.size();
  HandleData* hd =handle_thread_.service_handle_data_.at(tid);
  hd->lock_task_.lock();
  hd->task_.emplace(task);
  hd->lock_task_.unlock();
}

void ServiceManager::SendToSlaveThreadInner(EventMessage* msg){
  agents_->PutMessageToHandle(msg);
}

const std::string& ServiceManager::GetMonitorReport(){
  const static std::string k_null = "Null";

  monitor::Matrix::MatrixStatMapPtr stat_map = 
    monitor::GlobalMatrix::Instance().GetMatrixStatMap();
  if (!stat_map) {
    return k_null;
  } else {
    return stat_map->ToString();
  }
}

int ServiceManager::AgentsAddModrw(
  EventFunctionBase* ezfd, int fd,int& register_thread_id){
  return agents_->AddModrw(ezfd, fd, true,false,register_thread_id);
}

int ServiceManager::AgentsAddModr(
  EventFunctionBase* ezfd, int fd,int& register_thread_id){
  return agents_->AddModr(ezfd, fd,true,false,register_thread_id);
}

int ServiceManager::AgentsDel(EventFunctionBase* ezfd, int fd){
  return agents_->Del(ezfd, fd);
}

namespace service {
  ServiceManager& GetServiceManager() {
    return ServiceManager::GetInstance();
  }
}

}

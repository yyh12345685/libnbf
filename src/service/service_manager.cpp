
#include <functional>
#include <atomic>
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "service/service_handle.h"
#include "net/connect.h"
#include "message_base.h"
#include "monitor/matrix.h"
#include "monitor/matrix_stat_map.h"
#include "monitor/mem_profile.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ServiceManager);

int ServiceManager::Init(const ServiceConfig& config){
  net_thread_mgr_ = BDF_NEW(NetThreadManager,&config);
  if (!net_thread_mgr_->Init()){
    BDF_DELETE (net_thread_mgr_);
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
  if (!net_thread_mgr_->Start()){
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
  net_thread_mgr_->Stop();

  ThreadStop();
  //ThreadWait();

  if (net_thread_mgr_ != nullptr) {
    BDF_DELETE(net_thread_mgr_);
  }

  monitor::GlobalMatrix::Destroy();
  return 0;
}

int ServiceManager::Wait(){
  return ThreadWait();
}

void ServiceManager::SendCloseToNetThread(EventMessage* msg){
  msg->type_io_event = MessageType::kIoActiveCloseEvent;
  SendToNetThreadInner(msg);
}

void ServiceManager::Reply(EventMessage* message){
  message->direction = EventMessage::kOutgoingResponse;
  SendToNetThread(message);
}

void ServiceManager::SendToNetThread(EventMessage* msg){
  msg->type_io_event = MessageType::kIoHandleEventMsg;
  SendToNetThreadInner(msg);
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

void ServiceManager::SendToServiceHandle(EventMessage* msg) {
  uint32_t id = GetServiceHandleId(msg); //还是给到当时发送消息的线程
  TRACE(logger_, "ThreadId:" << ThreadId::Get()
    <<",handle id:" << msg->handle_id << ",Get id:" << id);
  HandleData* hd = handle_thread_.service_handle_data_.at(id);
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

void ServiceManager::SendTaskToServiceHandle(Task* task){
  static thread_local std::atomic<uint32_t> id_task(0);
  uint32_t tid = (id_task++) % handle_thread_.service_handle_data_.size();
  HandleData* hd = handle_thread_.service_handle_data_.at(tid);
  hd->lock_task_.lock();
  hd->task_.emplace(task);
  hd->lock_task_.unlock();
}

void ServiceManager::SendToNetThreadInner(EventMessage* msg){
  net_thread_mgr_->PutMessageToHandle(msg);
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

int ServiceManager::EventAddModrw(
  EventFunctionBase* ezfd, int fd,int& register_thread_id){
  return net_thread_mgr_->AddModrw(ezfd, fd, true,false,register_thread_id);
}

int ServiceManager::EventAddModr(
  EventFunctionBase* ezfd, int fd,int& register_thread_id){
  return net_thread_mgr_->AddModr(ezfd, fd,true,false,register_thread_id);
}

int ServiceManager::EventDel(EventFunctionBase* ezfd, int fd){
  return net_thread_mgr_->Del(ezfd, fd);
}

void ServiceManager::ReleaseServerCon(ServerConnect* svr_con){
  net_thread_mgr_->ReleaseServerCon(svr_con);
}

namespace service {
  ServiceManager& GetServiceManager() {
    return ServiceManager::GetInstance();
  }
}

}

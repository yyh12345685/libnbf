
#include <functional>
#include <atomic>
#include "service/io_service.h"
#include "agents/agents.h"
#include "service/service_handle.h"
#include "net/io_handle.h"
#include "net/connect.h"
#include "message_base.h"
#include "monitor/matrix.h"
#include "monitor/matrix_stat_map.h"
#include "monitor/mem_profile.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, IoService);

int IoService::Init(const IoServiceConfig& config){
  agents_ = BDF_NEW(Agents,&config);
  if (!agents_->Init()){
    BDF_DELETE (agents_);
    return -1;
  }

  monitor::GlobalMatrix::Init(config.monitor_file_name,
    config.monitor_token_bucket,
    config.monitor_queue_bucket,
    config.monitor_queue_size);

  io_service_config_ = config;
  return 0;
}

int IoService::Start(ServiceHandler* handle){
  if (!agents_->Start()){
    delete handle;
    return -1;
  }

  for (int cn = 0; cn < io_service_config_.service_handle_thread_count; cn++) {
    HandleData* hd = new HandleData;
    hd->handle_ = handle->Clone();
    hd->is_run = true;
    hd->handle_->SetHandlerId(cn);
    handle_thread_.service_handle_data_.push_back(hd);
    std::thread* thread = new std::thread(std::bind(&Handler::Run, hd->handle_, hd));
    handle_thread_.service_handle_thread_.push_back(thread);
  }

  for (int cn = 0; cn < io_service_config_.io_handle_thread_count; cn++) {
    HandleData* hd_io = new HandleData;
    hd_io->handle_ = new IoHandler();
    hd_io->is_run = true;
    hd_io->handle_->SetHandlerId(cn);
    handle_thread_.io_handle_data_.push_back(hd_io);
    std::thread* thread_io = new std::thread(std::bind(&Handler::Run, hd_io->handle_, hd_io));
    handle_thread_.io_handle_thread_.push_back(thread_io);
  }
  delete handle;
  return 0;
}

int IoService::ThreadWait(){
  INFO(logger_, "IoService::ThreadWait start.");
  for (int cn = 0; cn < io_service_config_.service_handle_thread_count; cn++){
    handle_thread_.service_handle_thread_[cn]->join();
  }

  for (int cn = 0; cn < io_service_config_.io_handle_thread_count; cn++) {
    handle_thread_.io_handle_thread_[cn]->join();
  }
  INFO(logger_, "IoService::ThreadWait end.");
  return 0;
}

int IoService::ThreadStop(){
  INFO(logger_, "IoService::ThreadStop.");
  for (int cn = 0; cn < io_service_config_.service_handle_thread_count; cn++){
    handle_thread_.service_handle_data_[cn]->is_run = false;
  }
  for (int cn = 0; cn < io_service_config_.io_handle_thread_count; cn++) {
    handle_thread_.io_handle_data_[cn]->is_run = false;
  }
  INFO(logger_, "IoService::Set Stope ok.");
  return 0;
}

int IoService::Stop(){
  agents_->Stop();

  ThreadStop();
  ThreadWait();

  if (agents_ != nullptr) {
    BDF_DELETE(agents_);
  }

  monitor::GlobalMatrix::Destroy();
  return 0;
}

int IoService::Wait(){
  return ThreadWait();
}

void IoService::SendCloseToIoHandle(EventMessage* msg){
  msg->type_io_event = MessageType::kIoActiveCloseEvent;
  SendToIoHandleInner(msg);
}

void IoService::Reply(EventMessage* message){
  message->direction = EventMessage::kOutgoingResponse;
  SendToIoHandle(message);
}

void IoService::SendToIoHandle(EventMessage* msg){
  msg->type_io_event = MessageType::kIoHandleEventMsg;
  SendToIoHandleInner(msg);
}

void IoService::SendEventToIoHandle(EventMessage* msg){
  msg->type_io_event = MessageType::kIoEvent;
  SendToIoHandleInner(msg);
}

uint32_t IoService::GetServiceHandleId(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_hs(0);
  if (msg->handle_id >=0 && 
    msg->handle_id < (int32_t)(handle_thread_.service_handle_thread_.size())){
    return msg->handle_id;
  }
  return (id_hs++) % handle_thread_.service_handle_data_.size();
}

uint32_t IoService::GetServiceHandleCount() {
  return handle_thread_.service_handle_thread_.size();
}

void IoService::SendToServiceHandle(EventMessage* msg){
  uint32_t id = GetServiceHandleId(msg);
  TRACE(logger_, "handle id:" << msg->handle_id << ",Get id:" << id);
  HandleData* hd = handle_thread_.service_handle_data_.at(id);
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

void IoService::SendTaskToServiceHandle(Task* task){
  static thread_local std::atomic<uint32_t> id_task(0);
  uint32_t tid = (id_task++) % handle_thread_.service_handle_data_.size();
  HandleData* hd =handle_thread_.service_handle_data_.at(tid);
  hd->lock_task_.lock();
  hd->task_.emplace(task);
  hd->lock_task_.unlock();
}

void IoService::SendToIoHandleInner(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_io(0);
  uint32_t id = 0;
  if (0!= msg->descriptor_id){
    Connecting* con = (Connecting*)((void*)(msg->descriptor_id));
    //不能用con指针地址取模，回导致线程的队列分布非常不均匀，使用顺序id即可
    id = con->GetConnectId() %handle_thread_.io_handle_data_.size();
  } else {
    id = (id_io++) % handle_thread_.io_handle_data_.size();
  }
  
  HandleData* hd =
    handle_thread_.io_handle_data_.at(id);
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

const std::string& IoService::GetMonitorReport(){
  const static std::string k_null = "Null";

  monitor::Matrix::MatrixStatMapPtr stat_map = 
    monitor::GlobalMatrix::Instance().GetMatrixStatMap();
  if (!stat_map) {
    return k_null;
  } else {
    return stat_map->ToString();
  }
}

int IoService::AgentsAddModrw(EventFunctionBase* ezfd, int fd){
  return agents_->AddModrw(ezfd, fd, true);
}

int IoService::AgentsAddModr(EventFunctionBase* ezfd, int fd){
  return agents_->AddModr(ezfd, fd,true);
}

int IoService::AgentsDel(EventFunctionBase* ezfd, int fd){
  return agents_->Del(ezfd, fd);
}

namespace service {
  IoService& GetIoService() {
    return IoService::GetInstance();
  }
}

}

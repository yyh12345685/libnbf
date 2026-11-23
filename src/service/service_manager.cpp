
#include <functional>
#include <atomic>
#include <memory>
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "service/service_handle.h"
#include "coroutine/coroutine.h"
#include "net/connect.h"
#include "message_base.h"
#include "monitor/matrix.h"
#include "monitor/matrix_stat_map.h"
#include "monitor/mem_profile.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ServiceManager);

static_assert(ATOMIC_BOOL_LOCK_FREE == 2 && ATOMIC_INT_LOCK_FREE == 2,
  "ServiceManager signal handling requires lock-free atomics");

int ServiceManager::Init(const ServiceConfig& config){
  if (net_thread_mgr_) return -1;
  if (config.coroutine_type != Coroutine::kCoroutineTypeC) {
    ERROR(logger_, "Unsupported coroutine_type: " << config.coroutine_type);
    return -1;
  }
  service_config_ = config;
  net_thread_mgr_ = BDF_NEW(NetThreadManager,&service_config_);
  if (!net_thread_mgr_->Init()){
    BDF_DELETE (net_thread_mgr_);
    net_thread_mgr_ = nullptr;
    return -1;
  }

  // The monitor is used unguarded everywhere (MatrixCounter, Invoke...), so
  // a failed init must fail startup instead of leaving a null instance.
  std::lock_guard<std::mutex> guard(monitor_lock_);
  if (0 != monitor::GlobalMatrix::Init(config.monitor_file_name,
    config.monitor_token_bucket,
    config.monitor_queue_bucket,
    config.monitor_queue_size)) {
    BDF_DELETE (net_thread_mgr_);
    net_thread_mgr_ = nullptr;
    return -2;
  }

  return 0;
}

int ServiceManager::Start(ServiceHandler* handle){
  return Start(handle, true);
}

int ServiceManager::Start(ServiceHandler* handle, bool start_acceptor){
  std::unique_ptr<ServiceHandler> prototype(handle);
  if (!net_thread_mgr_ || !handle || handlers_ready_) return -1;

  const int count = service_config_.service_handle_thread_count;
  try {
    handle_thread_.service_handle_data_.reserve(count);
    handle_thread_.service_handle_thread_.resize(count, nullptr);
    for (int index = 0; index < count; ++index) {
      std::unique_ptr<HandleData> data(new HandleData);
      data->handle_ = handle->Clone();
      if (!data->handle_) break;
      data->is_run = true;
      data->handle_->SetHandlerId(index);
      handle_thread_.service_handle_data_.push_back(data.release());
    }
    if (handle_thread_.service_handle_data_.size() == static_cast<size_t>(count)) {
      handlers_ready_ = true;
      if (stop_requested_) ThreadStopFromSignal();
      for (int index = 0; index < count; ++index) {
        HandleData* data = handle_thread_.service_handle_data_[index];
        handle_thread_.service_handle_thread_[index] =
          new std::thread(std::bind(&Handler::Run, data->handle_, data));
      }
      if (net_thread_mgr_->Start(start_acceptor)) return 0;
    }
  } catch (const std::exception& error) {
    ERROR(logger_, "service startup failed: " << error.what());
  }

  Stop();
  return -1;
}

int ServiceManager::ThreadWait(){
  INFO(logger_, "ServiceManager::ThreadWait start.");
  for (auto* thread : handle_thread_.service_handle_thread_){
    // Wait() and Stop() both join; joining twice throws std::system_error.
    if (thread && thread->joinable()) {
      thread->join();
    }
  }

  INFO(logger_, "ServiceManager::ThreadWait end.");
  return 0;
}

int ServiceManager::ThreadStop(){
  INFO(logger_, "ServiceManager::ThreadStop.");
  for (auto* data : handle_thread_.service_handle_data_){
    data->is_run = false;
  }

  INFO(logger_, "ServiceManager::Set Stope ok.");
  return 0;
}

void ServiceManager::ThreadStopFromSignal(){
  stop_requested_ = true;
  ++signal_readers_;
  if (handlers_ready_) {
    for (HandleData* data : handle_thread_.service_handle_data_) {
      data->is_run = false;
    }
  }
  --signal_readers_;
}

int ServiceManager::StopHandlers(){
  ThreadStop();
  ThreadWait();
  return 0;
}

int ServiceManager::Stop(){
  // Join the handler threads before freeing the net threads: they call
  // SendToNetThread until they exit.
  ThreadStop();
  ThreadWait();

  if (net_thread_mgr_ != nullptr) {
    net_thread_mgr_->Stop();
    BDF_DELETE(net_thread_mgr_);
    net_thread_mgr_ = nullptr;
  }
  ResetHandlers();

  std::lock_guard<std::mutex> guard(monitor_lock_);
  monitor::GlobalMatrix::Destroy();
  return 0;
}

void ServiceManager::ResetHandlers(){
  // I/O threads are joined. Unpublish the vector before waiting for any
  // signal handler that already started traversing it.
  handlers_ready_ = false;
  while (signal_readers_ != 0) std::this_thread::yield();
  for (auto* thread : handle_thread_.service_handle_thread_) {
    delete thread;
  }
  handle_thread_.service_handle_thread_.clear();
  for (auto* data : handle_thread_.service_handle_data_) {
    while (!data->data_.empty()) {
      MessageFactory::Destroy(data->data_.front());
      data->data_.pop();
    }
    delete data;
  }
  handle_thread_.service_handle_data_.clear();
  stop_requested_ = false;
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

void ServiceManager::DropRequest(EventMessage* message){
  if (message->direction != MessageBase::kIncomingRequest
    || 0 == message->descriptor_id || nullptr == net_thread_mgr_) {
    MessageFactory::Destroy(message);
    return;
  }
  message->event_mask |= MessageBase::kEventDropNotice;
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
    msg->handle_id < (int32_t)(handle_thread_.service_handle_data_.size())){
    return msg->handle_id;
  }
  return (id_hs++) % handle_thread_.service_handle_data_.size();
}

bool ServiceManager::HasServiceHandle(){
  return !handle_thread_.service_handle_data_.empty();
}

uint32_t ServiceManager::GetServiceHandleCount() {
  return handle_thread_.service_handle_data_.size();
}

void ServiceManager::SendToServiceHandle(EventMessage* msg){
  if (!HasServiceHandle()) {
    // DropRequest destroys non-requests and notifies the connection when an
    // incoming request will have no reply.
    WARN(logger_, "no service handle thread, drop msg type:" << (int)msg->type_id);
    DropRequest(msg);
    return;
  }
  uint32_t id = GetServiceHandleId(msg);
  TRACE(logger_, "ThreadId:" << ThreadId::Get()
    <<",handle id:" << msg->handle_id << ",Get id:" << id);
  HandleData* hd = handle_thread_.service_handle_data_.at(id);
  hd->data_lock_.lock();
  hd->data_.emplace(msg);
  hd->data_lock_.unlock();
}

void ServiceManager::SendTaskToServiceHandle(Task* task){
  static thread_local std::atomic<uint32_t> id_task(0);
  if (!HasServiceHandle()) {
    WARN(logger_, "no service handle thread, drop task");
    return;
  }
  uint32_t tid = (id_task++) % handle_thread_.service_handle_data_.size();
  HandleData* hd =handle_thread_.service_handle_data_.at(tid);
  hd->task_lock_.lock();
  hd->task_.emplace(task);
  hd->task_lock_.unlock();
}

void ServiceManager::SendToNetThreadInner(EventMessage* msg){
  net_thread_mgr_->PutMessageToHandle(msg);
}

std::string ServiceManager::GetMonitorReport(){
  const static std::string k_null = "Null";

  std::lock_guard<std::mutex> guard(monitor_lock_);
  if (!monitor::GlobalMatrix::Ready()) return k_null;
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


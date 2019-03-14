
#include <functional>
#include <signal.h>
#include <atomic>
#include "service/io_service.h"
#include "agents/agents.h"
#include "service/service_handle.h"
#include "net/io_handle.h"
#include "monitor/matrix.h"
#include "monitor/matrix_stat_map.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, IoService);

int IoService::Init(const IoServiceConfig& config){
  agents_ = new Agents(&config);
  if (!agents_->Init()){
    delete agents_;
    return -1;
  }

  io_service_config_ = config;
  return 0;
}

int IoService::Start(ServiceHandler* handle){
  if (!agents_->Start()){
    return -1;
  }

  for (int cn = 0; cn < io_service_config_.slave_thread_count;cn++) {
    HandleData* hd = new HandleData;
    hd->handle_ = handle->Clone();
    hd->is_run = true;
    handle_thread_.service_handle_data_.push_back(hd);
    std::thread* thread = new std::thread(std::bind(&ServiceHandler::Run, hd->handle_, hd));
    handle_thread_.service_handle_thread_.push_back(thread);

    HandleData* hd_io = new HandleData;
    hd_io->handle_ = new IoHandler();
    hd->is_run = true;
    handle_thread_.io_handle_data_.push_back(hd_io);
    std::thread* thread_io = new std::thread(std::bind(&IoHandler::Run, hd_io->handle_, hd_io));
    handle_thread_.io_handle_thread_.push_back(thread_io);
  }

  return 0;
}

int IoService::ThreadWait(){
  for (int cn = 0; cn < io_service_config_.slave_thread_count; cn++){
    handle_thread_.service_handle_thread_[cn]->join();
    handle_thread_.io_handle_thread_[cn]->join();
  }
  return 0;
}

int IoService::ThreadStop(){
  for (int cn = 0; cn < io_service_config_.slave_thread_count; cn++) {
    handle_thread_.service_handle_data_[cn]->is_run = false;
    handle_thread_.io_handle_data_[cn]->is_run = false;
  }
  return 0;
}

int IoService::Stop(){
  agents_->Stop();

  ThreadStop();
  ThreadWait();

  if (agents_ != nullptr) {
    delete agents_;
  }
  return 0;
}

int IoService::Wait(){
  return ThreadWait();
}

void IoService::SendToIoHandle(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_io(0);
  HandleData* hd =
    handle_thread_.io_handle_data_.at(id_io%handle_thread_.io_handle_data_.size());
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

void IoService::SendToIoHandleServiceHandle(EventMessage* msg){
  static thread_local std::atomic<uint32_t> id_hs(0);
  HandleData* hd = 
    handle_thread_.service_handle_data_.at(id_hs%handle_thread_.service_handle_data_.size());
  hd->lock_.lock();
  hd->data_.emplace(msg);
  hd->lock_.unlock();
}

const std::string& IoService::GetMatrixReport(){
  const static std::string k_null = "Null";

  monitor::Matrix::MatrixStatMapPtr stat_map = 
    monitor::GlobalMatrix::Instance().GetMatrixStatMap();
  if (!stat_map) {
    return k_null;
  } else {
    return stat_map->ToString();
  }
}

int IoService::AgentsAddModr(EventFunctionBase *ezfd, int fd, bool set) {
  return agents_->AddModr(ezfd, fd, set);
}

int IoService::AgentsModr(EventFunctionBase* ezfd, int fd, bool set) {
  return agents_->Modr(ezfd, fd, set);
}

int IoService::AgentsModw(EventFunctionBase* ezfd, int fd, bool set) {
  return agents_->Modw(ezfd, fd, set);
}

int IoService::AgentsDel(EventFunctionBase* ezfd, int fd){
  return agents_->Del(ezfd, fd);
}

void IoService::HandleSignal(){
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
  sigaction(SIGHUP, &sa, 0);
  sigaction(SIGCHLD, &sa, 0);
  signal(SIGINT, &IoService::StopIoService);
  signal(SIGTERM, &IoService::StopIoService);
}

void IoService::StopIoService(int signal){
  TRACE(logger, "service will be stop,signal:" << signal);
  GetInstance().Stop();
}

namespace service {
  IoService& GetIoService() {
    return IoService::GetInstance();
  }
}

}

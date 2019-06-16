#include <unistd.h>
#include <signal.h>
#include "app/appbase.h"
#include "client/client_mgr.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, AppBase);

AppBase* AppBase::application_ = nullptr;

AppBase::AppBase():
  config_info_(nullptr),
  client_mgr_(new ClientMgr()){}

AppBase::~AppBase() {
  if (config_info_) {
    delete config_info_;
    config_info_ = nullptr;
  }
  if (client_mgr_){
    delete client_mgr_;
    client_mgr_ = nullptr;
  }
}

int AppBase::Run(int argc, char* argv[]) {
  if (0 != Start(argc, argv)) {
    return -1;
  }

  if (0 != Wait()) {
    return -1;
  }

  Stop();
  return 0;
}

int AppBase::Start(int argc, char* argv[]) {
  if (0 != cmd_parser_.ParseCmd(argc, argv)) {
    return -1;
  }

  if (0 != StartLogger()) {
    return -1;
  }

  if (0 != InitApplication()) {
    return -1;
  }

  if (0 != LoadConfig()) {
    return -1;
  }

  HandleSignal();

  if (0 != OnStart()) {
    return -1;
  }

  if (0 != StartIoService()) {
    return -1;
  }

  if (0 != StartClientManager()){
    return -1;
  }

  if (0 != OnAfterStart()) {
    return -1;
  }

  return 0;
}

int AppBase::Wait() {
  if (0 != WaitIoService()) {
    return -1;
  }
  return 0;
}

int AppBase::Stop() {
  INFO(logger_, "AppBase::OnStop");
  OnStop();

  INFO(logger_, "AppBase::StopClientManager");
  StopClientManager();

  INFO(logger_, "AppBase::StopIoService");
  StopIoService();

  return 0;
}

int AppBase::InitApplication() {
  AppBase::Set(this);

  if (cmd_parser_.daemon_mode_) {
    daemon(1, 0);
  }

  return 0;
}

int AppBase::StartLogger() {
  if (cmd_parser_.logger_config_.empty()){
    return -1;
  }

  LOGGER_SYS_INIT_DYNAMIC(watcher, cmd_parser_.logger_config_,10000);
  return 0;
}

int AppBase::LoadConfig() {
  if (cmd_parser_.application_config_.empty()){
    return -1;
  }

  ConfigInfo* config_info = new ConfigInfo();
  if (0 != config_info->LoadConfig(cmd_parser_.application_config_)) {
    delete config_info;
    return -1;
  }
  io_service_config_ = config_info->GetIOServiceConfig();
  config_info_ = config_info;

  return 0;
}

int AppBase::StartIoService() {
  TRACE(logger_, "AppBase::StartIoService().");
  if (0 != IoService::GetInstance().Init(io_service_config_)) {
    ERROR(logger_, "Application::StartIOService Init fail");
    return -1;
  }

  ServiceHandler* handle_tmp = CreateServiceHandler();
  if (0 != IoService::GetInstance().Start(handle_tmp)) {
    ERROR(logger_, "Application::StartIOService Start fail");
    return -1;
  }

  return 0;
}

int AppBase::StartClientManager(){
  return client_mgr_->Start(config_info_->GetClientRoutersConfig());
}

void AppBase::HandleSignal() {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
  sigaction(SIGHUP, &sa, 0);
  sigaction(SIGCHLD, &sa, 0);
  signal(SIGINT, &AppBase::StopApp);
  signal(SIGTERM, &AppBase::StopApp);
}

void AppBase::StopApp(int signal) {
  TRACE(logger_, "service will be stop,signal:" << signal);
  AppBase::Get()->Stop();
}

int AppBase::StopClientManager(){
  return client_mgr_->Stop();
}

int AppBase::WaitIoService() {
  IoService::GetInstance().Wait();
  return 0;
}

int AppBase::StopIoService() {
  IoService::GetInstance().Stop();
  return 0;
}

}

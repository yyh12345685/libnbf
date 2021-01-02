#include <unistd.h>
#include <signal.h>
#include "app/appbase.h"
#include "client/client_mgr.h"
#include "service/service_manager.h"

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

int AppBase::AppRun(int argc, char* argv[]) {
  if (0 != AppStart(argc, argv)) {
    return -1;
  }

  if (0 != AppWait()) {
    return -1;
  }

  AppStop();
  return 0;
}

int AppBase::AppStart(int argc, char* argv[]) {
  if (0 != cmd_parser_.ParseCmd(argc, argv)) {
    return -1;
  }

  srand(time(nullptr));
  if (0 != StartLogger()) {
    return -1;
  }

  if (0 != SetApplication()) {
    return -1;
  }

  if (0 != LoadConfig()) {
    return -1;
  }

  HandleSignal();

  if (0 != OnStart()) {
    return -1;
  }

  if (0 != StartService()) {
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

int AppBase::AppWait() {
  if (0 != WaitService()) {
    return -1;
  }
  return 0;
}

int AppBase::AppStop() {
  INFO(logger_, "AppBase::OnStop");
  OnStop();

  INFO(logger_, "AppBase::StopClientManager");
  StopClientManager();

  INFO(logger_, "AppBase::StopService");
  StopService();

  return 0;
}

int AppBase::SetApplication() {
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
  io_service_config_ = config_info->GetServiceConfig();
  config_info_ = config_info;

  return 0;
}

int AppBase::StartService() {
  TRACE(logger_, "AppBase::StartService().");
  if (0 != service::GetServiceManager().Init(io_service_config_)) {
    ERROR(logger_, "Application::StartService Init fail");
    return -1;
  }

  ServiceHandler* handle_tmp = CreateServiceHandler();
  if (0 != service::GetServiceManager().Start(handle_tmp)) {
    ERROR(logger_, "Application::StartService Start fail");
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
  INFO(logger_, "service will be Stop,signal:" << signal);
  AppBase::Get()->AppStop();
}

int AppBase::StopClientManager(){
  return client_mgr_->Stop();
}

int AppBase::WaitService() {
  service::GetServiceManager().Wait();
  return 0;
}

int AppBase::StopService() {
  service::GetServiceManager().Stop();
  return 0;
}

}

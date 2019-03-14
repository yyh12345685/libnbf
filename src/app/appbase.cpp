#include <unistd.h>
#include "app/appbase.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AppBase);

AppBase* AppBase::application_ = NULL;

AppBase::AppBase()
  : config_info_(nullptr){}

AppBase::~AppBase() {
  if (config_info_) {
    delete config_info_;
    config_info_ = nullptr;
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

  if (0 != StartIoService()) {
    return -1;
  }

  if (0 != OnStart()) {
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
  INFO(logger, "Applciation::OnStop");

  OnStop();
  INFO(logger, "Applciation::StopIOService");
  StopIoService();

  INFO(logger, "Applciation::OnFinish");
  OnFinish();

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
  TRACE(logger, "AppBase::StartIoService().");
  IoService::GetInstance().HandleSignal();

  if (0 != IoService::GetInstance().Init(io_service_config_)) {
    ERROR(logger, "Application::StartIOService Init fail");
    return -1;
  }

  if (0 != IoService::GetInstance().Start(CreateServiceHandler())) {
    ERROR(logger, "Application::StartIOService Start fail");
    return -1;
  }

  return 0;
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

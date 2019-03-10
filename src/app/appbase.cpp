#include "app/appbase.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AppBase);

AppBase* AppBase::application_ = NULL;

AppBase::AppBase()
  , config_manager_(NULL)
  , client_manager_(new ClientManager(GetIOService()))
  , start_time_(time(NULL))
  , help_mode_(false)
  , daemon_mode_(false) {}

AppBase::~AppBase() {
  if (config_manager_) {
    delete config_manager_;
    config_manager_ = NULL;
  }
  if (client_manager_) {
    delete client_manager_;
    client_manager_ = NULL;
  }
}

int AppBase::Run(int argc, char* argv[], const std::string& version) {
  if (0 != Start(argc, argv, version)) {
    return -1;
  }

  if (help_mode_) {
    return 0;
  }

  if (0 != Wait()) {
    return -1;
  }

  Stop();
  return 0;
}

int AppBase::Start(int argc, char* argv[]) {

  if (0 != ParseCmd(argc, argv)) {
    return -1;
  }

  if (help_mode_ ) {
    return 0;
  }

  if (0 != InitApplication()) {
    return -1;
  }

  WritePidFile();

  if (0 != StartLogger()) {
    return -1;
  }

  if (0 != LoadConfig()) {
    return -1;
  }

  DumpStatus(std::cout);

  if (0 != StartIOService()) {
    return -1;
  }

  if (0 != StartClientManager()) {
    return -1;
  }

  if (0 != OnStart()) {
    return -1;
  }

  if (0 != StartServiceManager()) {
    return -1;
  }

  return 0;
}

int AppBase::Wait() {
  if (0 != RunIOService()) {
    return -1;
  }
  return 0;
}

int AppBase::Stop() {
  MI_LOG_INFO(logger, "Applciation::OnStop");

  OnStop();

  MI_LOG_INFO(logger, "Applciation::StopServiceManager");

  StopServiceManager();

  MI_LOG_INFO(logger, "Applciation::StopClientManager");

  StopClientManager();

  sleep(5);

  MI_LOG_INFO(logger, "Applciation::StopIOService");

  StopIOService();

  MI_LOG_INFO(logger, "Applciation::OnFinish");

  OnFinish();

  return 0;
}

int AppBase::ParseCmd(int argc, char* argv[]) {
  
  return 0;
}

int AppBase::InitApplication() {
  AppBase::Set(this);

  if (daemon_mode_) {
    daemon(1, 0);
  }

  return 0;
}

int AppBase::WritePidFile() {
  std::ofstream ofs;
  ofs.open("pid", std::ios::trunc | std::ios::out);
  if (!ofs) {
    std::cerr << "write pid file error" << std::endl;
    return -1;
  }
  ofs << getpid();
  return 0;
}

int AppBase::StartLogger() {
  if (logger_config_.empty()) {
    std::cout << "Initialize logger with default configuration" << std::endl;

    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender());
    std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout("%D{[%Y/%m/%d-%H:%M:%S]} (%t) %-5p%x - %m [%l]%n"));
    appender->setLayout(layout);
    log4cplus::Logger logger = log4cplus::Logger::getRoot();
    logger.addAppender(appender);
    logger.setLogLevel(log4cplus::TRACE_LOG_LEVEL);
  } else {
    logger_watcher_ = new log4cplus::ConfigureAndWatchThread(logger_config_, 5000);
  }
  return 0;
}

int AppBase::LoadConfig() {
  if (!application_config_.empty()) {
    ConfigManager* config_manager = CreateConfigManager();
    if (0 != config_manager->LoadConfig(application_config_)) {
      return -1;
    }
    io_service_config_ = config_manager->GetIOServiceConfig();
    config_manager_ = config_manager;
  } else {
    MI_LOG_INFO(logger, "Load with default configuration");
    io_service_config_.event_loop_worker = 1;
    io_service_config_.fd_count = 65535;
    io_service_config_.io_queue_size = 128 * 1024;
    io_service_config_.io_worker = 1;
    io_service_config_.service_queue_size = 128 * 1024;
    io_service_config_.service_worker = 1;
    io_service_config_.service_timer_worker = 0;
    io_service_config_.service_timer_queue_size = 0;
    io_service_config_.stack_size = 1024 * 16;
  }

  io_service_config_.service_handler_prototype = CreateServiceHandler();

  return 0;
}

int AppBase::StartIOService() {
  io_service_.HandleSignal();

  if (0 != io_service_.Init(io_service_config_)) {
    MI_LOG_ERROR(logger, "Application::StartIOService Init fail");
    return -1;
  }

  if (0 != io_service_.Start()) {
    MI_LOG_ERROR(logger, "Application::StartIOService Start fail");
    return -1;
  }

  return 0;
}

int AppBase::StartClientManager() {
  if (!config_manager_) {
    MI_LOG_DEBUG(logger, "Application::StartClientManager no config set, ignore");
  } else {
    for (const auto& router_config : config_manager_->GetClientRoutersConfig()) {
      if (0 != client_manager_->AddClient(router_config)) {
        return -1;
      }
    }
  }

  if (0 != client_manager_->Start()) {
    LOG_ERROR(logger, "Application::StartClientManager Start fail");
    return -1;
  }

  return 0;
}

int AppBase::StartServiceManager() {
  if (!config_info_) {
    MI_LOG_DEBUG(logger, "Application::StartServiceManager no config set, ignore");
    return 0;
  }

  return 0;
}

int AppBase::RunIOService() {
  io_service_.Run();
  return 0;
}


int AppBase::StopClientManager() {
  if (client_manager_) {
    return client_manager_->Stop();
  }
  return 0;
}

int AppBase::StopIOService() {
  io_service_.CleanUp();
  return 0;
}

}

#include <unistd.h>
#include <signal.h>
#include <atomic>
#include "app/appbase.h"
#include "client/client_mgr.h"
#include "service/service_manager.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, AppBase);

// The user's OnStop hook runs exactly once: from the signal handler (existing
// apps rely on it to end loops started in OnAfterStart) or from AppStop.
static std::atomic<bool> g_on_stop_called(false);
static void CallOnStopOnce(AppBase* app) {
  if (!g_on_stop_called.exchange(true)) {
    app->OnStopHook();
  }
}

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
    AppStop(); // net threads may already be running
    return -1;
  }

  // From here on threads are running: a failure must stop them before
  // returning, or ~AppBase and static destructors free objects those threads
  // still use and destroy joinable std::threads (std::terminate).
  if (0 != StartClientManager()){
    AppStop();
    return -1;
  }

  if (0 != OnAfterStart()) {
    AppStop();
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
  // Runs once: AppRun calls it after AppWait returns, and user code may too.
  static std::atomic<bool> stopped(false);
  if (stopped.exchange(true)) {
    return 0;
  }
  INFO(logger_, "AppBase::OnStop");
  CallOnStopOnce(this);

  // Join the handler threads before stopping clients: a handler that is
  // still running could hand a request to a client connection that is being
  // destroyed on its I/O loop.
  INFO(logger_, "AppBase::StopHandlers");
  service::GetServiceManager().StopHandlers();

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
  // Signal handler: no logging, locks or deletes here (it used to run the
  // whole AppStop, then AppRun ran it a second time on freed objects). Only
  // ask the handler threads to exit; AppWait then returns and AppRun stops
  // everything once on the main thread.
  (void)signal;
  CallOnStopOnce(AppBase::Get()); // must itself be signal-safe (e.g. set an atomic)
  service::GetServiceManager().ThreadStopFromSignal();
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

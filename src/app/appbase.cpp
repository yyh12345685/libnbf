#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <system_error>
#include "app/appbase.h"
#include "client/client_mgr.h"
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, AppBase);

static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "Signal notification requires lock-free atomics");
static std::atomic<bool> stop_signal_received{false};

void AppBase::CallOnStopOnce() {
  if (!on_stop_called_.exchange(true)) {
    OnStopHook();
  }
}

AppBase* AppBase::application_ = nullptr;

AppBase::AppBase():
  config_info_(nullptr),
  client_mgr_(new ClientMgr()){}

AppBase::~AppBase() {
  StopSignalThread();
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

  if (0 != HandleSignal()) {
    return -1;
  }

  if (0 != OnStart()) {
    StopSignalThread();
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

  // Publish the client routes before dispatching incoming requests.
  if (!service::GetServiceManager().GetNetThreadManager()->StartAccepting()) {
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
  if (stopped_.exchange(true)) {
    return 0;
  }
  INFO(logger_, "AppBase::OnStop");
  StopSignalThread();
  CallOnStopOnce();

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

ConfigInfo* AppBase::CreateConfig() {
  ConfigInfo* config_info = new ConfigInfo();
  return config_info;
}

int AppBase::LoadConfig() {
  if (cmd_parser_.application_config_.empty()){
    return -1;
  }

  ConfigInfo* config_info = CreateConfig();
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
  if (0 != service::GetServiceManager().Start(handle_tmp, false)) {
    ERROR(logger_, "Application::StartService Start fail");
    return -1;
  }

  return 0;
}

int AppBase::StartClientManager(){
  return client_mgr_->Start(config_info_->GetClientRoutersConfig());
}

int AppBase::HandleSignal() {
  stop_signal_received = false;
  signal_thread_done_ = false;
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
  sigaction(SIGHUP, &sa, 0);
  sigaction(SIGCHLD, &sa, 0);
  sa.sa_handler = &AppBase::StopApp;
  if (sigaction(SIGINT, &sa, nullptr) < 0 || sigaction(SIGTERM, &sa, nullptr) < 0) {
    ERROR(logger_, "Cannot install stop signal handlers");
    return -1;
  }
  try {
    signal_thread_ = std::thread([this] {
      while (!signal_thread_done_.load()) {
        if (stop_signal_received.load()) {
          CallOnStopOnce();
          service::GetServiceManager().ThreadStopFromSignal();
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  } catch (const std::system_error& error) {
    ERROR(logger_, "Cannot start signal thread: " << error.what());
    return -1;
  }
  return 0;
}

void AppBase::StopSignalThread() {
  signal_thread_done_ = true;
  if (signal_thread_.joinable() && signal_thread_.get_id() != std::this_thread::get_id()) {
    signal_thread_.join();
  }
}

void AppBase::StopApp(int) {
  stop_signal_received.store(true);
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

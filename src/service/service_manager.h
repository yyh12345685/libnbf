#pragma once

#include <vector>
#include <thread>
#include "common/common.h"
#include "app/config_info.h"
#include "handle_data.h"

namespace bdf{

class EventMessage;
class NetThreadManager;
class ServerConnect;
class ServiceHandler;
class EventFunctionBase;

struct HandleThread{
  std::vector<std::thread*>  service_handle_thread_;
  std::vector < HandleData*> service_handle_data_;
  
  ~HandleThread(){
    for (const auto& hd: service_handle_data_){
      delete hd;
    }
  
    for (const auto& thread : service_handle_thread_) {
      delete thread;
    }
  
  }
};

class ServiceManager {
public:

  static ServiceManager& GetInstance() {
    static ServiceManager inst;
    return inst;
  }

  int Init(const ServiceConfig& config);
  int Start(ServiceHandler* handle);
  int Start(ServiceHandler* handle, bool start_acceptor);
  int Stop();
  int Wait();
  // Async-signal-safe: uses lock-free atomics.
  void ThreadStopFromSignal();
  // Stop and join the service handler threads (idempotent).
  int StopHandlers();

  void Reply(EventMessage* message);
  // Drop a server request without replying. Use instead of Destroy for
  // incoming requests, so the connection knows it will get no reply.
  void DropRequest(EventMessage* message);
  void SendToNetThread(EventMessage* msg);
  void SendCloseToNetThread(EventMessage* msg);
  void SendToServiceHandle(EventMessage* msg);
  void SendTaskToServiceHandle(Task* task);

  std::string GetMonitorReport();

  int EventAddModrw(EventFunctionBase* ezfd, int fd,int& register_thread_id);
  int EventAddModr(EventFunctionBase* ezfd, int fd,int& register_thread_id);
  int EventDel(EventFunctionBase* ezfd, int fd);

  uint32_t GetServiceHandleCount();

  const ServiceConfig& GetServiceConfig(){
    return service_config_;
  }

  NetThreadManager* GetNetThreadManager(){
    return net_thread_mgr_;
  }

  void ReleaseServerCon(ServerConnect* svr_con);

private:
  void SendToNetThreadInner(EventMessage* msg);

  uint32_t GetServiceHandleId(EventMessage* msg);

  int ThreadWait();
  int ThreadStop();
  bool HasServiceHandle();
  void ResetHandlers();
  LOGGER_CLASS_DECL(logger_);

  ServiceConfig service_config_;

  HandleThread handle_thread_;

  std::atomic<bool> handlers_ready_{false};
  std::atomic<bool> stop_requested_{false};
  std::atomic<unsigned> signal_readers_{0};
  std::mutex monitor_lock_;
  NetThreadManager* net_thread_mgr_ = nullptr;

  DISALLOW_COPY_ASSIGN_CONSTRUCTION(ServiceManager)
};

namespace service {
  ServiceManager& GetServiceManager();
}

}


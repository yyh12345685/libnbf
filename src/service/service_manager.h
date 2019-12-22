#pragma once

#include <vector>
#include <thread>
#include "common/common.h"
#include "app/config_info.h"
#include "handle_data.h"

namespace bdf{

class EventMessage;
class Agents;
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
  int Stop();
  int Wait();

  void Reply(EventMessage* message);
  void SendToSlaveThread(EventMessage* msg);
  void SendCloseToSlaveThread(EventMessage* msg);
  void SendToServiceHandle(EventMessage* msg);
  void SendTaskToServiceHandle(Task* task);

  const std::string& GetMonitorReport();

  int AgentsAddModrw(EventFunctionBase* ezfd, int fd,int& register_thread_id);
  int AgentsAddModr(EventFunctionBase* ezfd, int fd,int& register_thread_id);
  int AgentsDel(EventFunctionBase* ezfd, int fd);

  uint32_t GetServiceHandleCount();

  const ServiceConfig& GetServiceConfig(){
    return service_config_;
  }

  Agents* GetAgents(){
    return agents_;
  }

private:
  void SendToSlaveThreadInner(EventMessage* msg);

  uint32_t GetServiceHandleId(EventMessage* msg);

  int ThreadWait();
  int ThreadStop();
  LOGGER_CLASS_DECL(logger_);

  ServiceConfig service_config_;

  HandleThread handle_thread_;

  Agents* agents_;

  DISALLOW_COPY_ASSIGN_CONSTRUCTION(ServiceManager)
};

namespace service {
  ServiceManager& GetServiceManager();
}

}


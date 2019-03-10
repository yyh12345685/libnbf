#pragma once

#include <functional>
#include "app/config_info.h"

namespace bdf{

class ServiceHandler;
class ClientManager;
class ConfigInfo;
class Agents;
class IOService;

class AppBase{

public:
  template<typename App = AppBase>
  inline static App* Get() {
    return (App*)application_;
  }

  inline static void Set(AppBase* app) {
    application_ = app;
  }

  AppBase();
  virtual ~AppBase();

  IOService* GetIOService() { return &io_service_; }
  ConfigInfo* GetConfigInfo() { return config_info_; }

  ClientManager* GetClientManager() { return client_manager_; }

  int Run(int argc, char* argv[]);
  int Start(int argc, char* argv[]);
  int Wait();
  int Stop();

  inline uint32_t GetStartTime() { return start_time_; }

  virtual void DumpStatus(std::ostream& os) const;

protected:
  virtual int OnStart() = 0;

  virtual int OnStop() = 0;

  virtual int OnFinish() = 0;

  virtual ServiceHandler* CreateServiceHandler() = 0;

  virtual ConfigInfo* CreateConfigInfo() = 0;

private:
  LOGGER_CLASS_DECL(logger);

  int ParseCmd(int argc, char* argv[]);

  int InitApplication();

  int WritePidFile();

  int StartLogger();

  int LoadConfig();

  int StartIOService();

  int StartClientManager();

  int StartServiceManager();

  int RunIOService();

  int StopClientManager();

  int StopServiceManager();

  int StopService();

  int StopIOService();

  static AppBase* application_;

  IOService io_service_;
  IOServiceConfig io_service_config_;

  ConfigInfo* config_info_;
  ClientManager* client_manager_;

  uint32_t start_time_;

  bool help_mode_;
  bool version_mode_;
  bool daemon_mode_;
  std::string version_info_;
  std::string application_config_;
  std::string logger_config_;

  Agents* agents_;
};

}

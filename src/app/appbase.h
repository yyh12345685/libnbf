#pragma once

#include <functional>
#include "app/config_info.h"
#include "app/app_cmd_parser.h"
#include "service/io_service.h"

namespace bdf{

class ServiceHandler;
class ClientMgr;
class ConfigInfo;

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

  ConfigInfo* GetConfigInfo() { return config_info_; }

  ClientMgr* GetClientMgr() { return client_mgr_; }

  int Run(int argc, char* argv[]);
  int Start(int argc, char* argv[]);
  int Wait();
  int Stop();

protected:
  virtual int OnStart() = 0;
  virtual int OnAfterStart() = 0;
  virtual int OnStop() = 0;

  virtual ServiceHandler* CreateServiceHandler() = 0;

private:
  LOGGER_CLASS_DECL(logger_);

  int InitApplication();

  int StartLogger();
  int LoadConfig();
  int StartIoService();

  int StartClientManager();

  int WaitIoService();

  int StopClientManager();
  int StopIoService();

  void HandleSignal();
  static void StopApp(int signal);

  static AppBase* application_;

  IoServiceConfig io_service_config_;

  ConfigInfo* config_info_;

  ClientMgr* client_mgr_;

  AppCmdParser cmd_parser_;
};

}

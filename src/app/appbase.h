#pragma once

#include <functional>
#include "app/config_info.h"
#include "app/app_cmd_parser.h"

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

  int AppRun(int argc, char* argv[]);
protected:
  int AppStart(int argc, char* argv[]);
  int AppWait();
  int AppStop();

protected:
  virtual int OnStart() = 0;
  virtual int OnAfterStart() = 0;
  virtual int OnStop() = 0;

  virtual ServiceHandler* CreateServiceHandler() = 0;

private:
  LOGGER_CLASS_DECL(logger_);

  int SetApplication();

  int StartLogger();
  int LoadConfig();
  int StartService();

  int StartClientManager();

  int WaitService();

  int StopClientManager();
  int StopService();

  void HandleSignal();
  static void StopApp(int signal);

  static AppBase* application_;

  ServiceConfig io_service_config_;

  ConfigInfo* config_info_;

  ClientMgr* client_mgr_;

  AppCmdParser cmd_parser_;
};

}

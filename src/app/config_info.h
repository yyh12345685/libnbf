#pragma once

#include <vector>
#include <string>
#include "common/logger.h"

namespace bdf{

class CIniFileS;

struct ServiceConfig {
  std::string address;
  std::string name;
};

struct IOServiceConfig {
  IOServiceConfig()
    : slave_count(65535)
    , stack_size(2048)
    , monitor_token_bucket(16)
    , monitor_queue_bucket(16)
    , monitor_queue_size(1024 * 512) {
  }

  int slave_count;

  // coroutine 
  uint32_t stack_size;

  // monitor
  uint32_t monitor_token_bucket;
  uint32_t monitor_queue_bucket;
  uint32_t monitor_queue_size;

  void Dump(std::ostream& os) const;
};

struct ClientConfig {
  std::string address;
  uint32_t timeout;
  uint32_t heartbeat;
  int count;
};

struct ClientRouterConfig {
  std::string name;
  std::string router;
  std::vector<ClientConfig> clients;
};

class ConfigInfo {
public:
  typedef std::vector<ServiceConfig> ServicesConfig;
  typedef std::vector<ClientRouterConfig> ClientRoutersConfig;

  ConfigInfo();
  virtual ~ConfigInfo();

  int LoadConfig(const std::string& config_path);

  IOServiceConfig& GetIOServiceConfig() { return io_service_config_; }

  ServicesConfig GetServicesConfig() { return services_config_; }

  ClientRoutersConfig GetClientRoutersConfig() { return client_routers_config_; }

  virtual void Dump(std::ostream& os) const;

protected:
  virtual int LoadIOServiceConfig(CIniFileS& ini, IOServiceConfig* config);

  virtual int LoadApplicationConfig(CIniFileS& ini);

  virtual int LoadServicesConfig(CIniFileS& ini, ServicesConfig* config);

  virtual int LoadClientRoutersConfig(CIniFileS& ini, ClientRoutersConfig* config);

private:
  LOGGER_CLASS_DECL(logger);

  IOServiceConfig io_service_config_;
  ServicesConfig services_config_;
  ClientRoutersConfig client_routers_config_;
};


}

}

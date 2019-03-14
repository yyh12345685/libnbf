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

struct IoServiceConfig {
  IoServiceConfig()
    : slave_thread_count(3)
    , service_count(1)
    , stack_size(2048)
    , monitor_token_bucket(16)
    , monitor_queue_bucket(16)
    , monitor_queue_size(1024 * 512) {
  }

  int slave_thread_count;
  int service_count;
  std::vector<ServiceConfig> services_config;

  // coroutine 
  uint32_t stack_size;

  // monitor
  uint32_t monitor_token_bucket;
  uint32_t monitor_queue_bucket;
  uint32_t monitor_queue_size;

  void Dump(std::ostream& os) const;
};

std::ostream& operator << (std::ostream& os, const IoServiceConfig& config);

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
  typedef std::vector<ClientRouterConfig> ClientRoutersConfig;

  ConfigInfo();
  virtual ~ConfigInfo();

  int LoadConfig(const std::string& config_path);

  IoServiceConfig& GetIOServiceConfig() { return io_service_config_; }

  ClientRoutersConfig GetClientRoutersConfig() { return client_routers_config_; }

  virtual void Dump(std::ostream& os) const;

protected:
  virtual int LoadIoServiceConfig(CIniFileS& ini, IoServiceConfig* config);

  virtual int LoadApplicationConfig(CIniFileS& ini);

  virtual int LoadServicesConfig(CIniFileS& ini, int srv_cnt,std::vector<ServiceConfig>& config);

private:
  LOGGER_CLASS_DECL(logger_);

  IoServiceConfig io_service_config_;
  ClientRoutersConfig client_routers_config_;
};

std::ostream& operator << (std::ostream& os, const ConfigInfo& config);

}


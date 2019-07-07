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
  IoServiceConfig(): 
    slave_thread_count(2),
    io_handle_thread_count(2),
    service_handle_thread_count(2),
    service_count(1),
    stack_size(2048),
    monitor_token_bucket(16), 
    monitor_queue_bucket(16), 
    monitor_queue_size(1024 * 128) {
  }

  int slave_thread_count;
  int io_handle_thread_count;
  int service_handle_thread_count;
  int service_count;
  std::vector<ServiceConfig> services_config;

  std::string router_file;

  // coroutine 
  uint32_t stack_size;

  // monitor
  std::string monitor_file_name;
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
  int single_addr_connect_count;
};

struct ClientRouterConfig {
  std::string name;
  std::string mapping;
  bool sigle_send_sigle_recv = false;
  //每个ClientConfig可以有不一样的address
  std::vector<ClientConfig> clients;
};

struct ClientRoutersConfig {
  std::vector<ClientRouterConfig> client_router_config;
  void Dump(std::ostream& os) const;
};

class ConfigInfo {
public:
  ConfigInfo();
  virtual ~ConfigInfo();

  int LoadConfig(const std::string& config_path);

  const IoServiceConfig& GetIOServiceConfig() { return io_service_config_; }
  const ClientRoutersConfig& GetClientRoutersConfig() { return client_routers_config_; }
  virtual void Dump(std::ostream& os) const;

protected:
  virtual int LoadIoServiceConfig(CIniFileS& ini, IoServiceConfig* config);
  virtual int LoadServicesConfig(CIniFileS& ini, int srv_cnt,std::vector<ServiceConfig>& config);
  virtual int LoadRouterConfig(const std::string& file_path);

  virtual int LoadApplicationConfig(CIniFileS& ini);

private:
  LOGGER_CLASS_DECL(logger_);

  IoServiceConfig io_service_config_;
  ClientRoutersConfig client_routers_config_;
};

std::ostream& operator << (std::ostream& os, const ConfigInfo& config);


}


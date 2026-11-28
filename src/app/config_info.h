#pragma once

#include <vector>
#include <string>
#include "common/logger.h"

namespace bdf{

class CIniFileS;

struct ServerConfig {
  std::string address;
  std::string name;
};

struct ServiceConfig {
  ServiceConfig(): 
    io_thread_count(2),
    service_handle_thread_count(2),
    service_count(1),
    coroutine_size(256),
    coroutine_type(0),
    coroutine_stack_size(0),
    max_coroutine_size(10000),
    client_send_queue_limit(30000),
    client_max_inflight(3000),
    monitor_token_bucket(16),
    monitor_queue_bucket(16), 
    monitor_queue_size(1024 * 128) {
  }

  int io_thread_count;
  int service_handle_thread_count;
  int service_count;
  std::vector<ServerConfig> server_config;

  std::string router_file;

  // coroutine 
  uint32_t coroutine_size;
  uint32_t coroutine_type;
  int coroutine_stack_size;
  // Soft cap per handler thread: once this many coroutines are suspended
  // (waiting on downstream calls), new incoming requests are shed instead
  // of creating more coroutines, so overload protection still works.
  uint32_t max_coroutine_size;

  // Client backpressure: Send/Invoke/SendRecieve fail fast once the I/O
  // thread owning the connection has this many messages queued. Without a
  // bound an open-loop caller grows the queue (and memory) without limit
  // while the I/O thread only drops what it eventually dequeues. 0 = off.
  uint32_t client_send_queue_limit;

  // Client backpressure for Invoke: at most this many requests per client
  // connection between Invoke and the destruction of their response
  // message (callback done). This bounds every queue on the way, including
  // the service-handler queue that runs callbacks, which has no age-based
  // shedding. Invoke fails fast (kInvokeError) beyond it. 0 = off.
  uint32_t client_max_inflight;

  // monitor
  std::string monitor_file_name;
  uint32_t monitor_token_bucket;
  uint32_t monitor_queue_bucket;
  uint32_t monitor_queue_size;

  void Dump(std::ostream& os) const;
};

std::ostream& operator << (std::ostream& os, const ServiceConfig& config);

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

  const ServiceConfig& GetServiceConfig() { return service_config_; }
  const ClientRoutersConfig& GetClientRoutersConfig() { return client_routers_config_; }
  virtual void Dump(std::ostream& os) const;

protected:
  virtual int LoadServiceConfig(CIniFileS& ini, ServiceConfig* config);
  virtual int LoadServicesConfig(CIniFileS& ini, int srv_cnt,std::vector<ServerConfig>& config);
  virtual int LoadRouterConfig(const std::string& file_path);

  virtual int LoadApplicationConfig(CIniFileS& ini);

private:
  LOGGER_CLASS_DECL(logger_);

  ServiceConfig service_config_;
  ClientRoutersConfig client_routers_config_;
};

std::ostream& operator << (std::ostream& os, const ConfigInfo& config);


}


#include "app/config_info.h"
#include "common/ini_files.h"
#include "common/string_util.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ConfigInfo)

void ServiceConfig::Dump(std::ostream& os) const {
  os << "{\"type\": \"ServiceConfig\""
    << ", \"slave_thread_count\": " << slave_thread_count
    << ", \"service_handle_thread_count\": " << service_handle_thread_count
    << ", \"service_count\": " << service_count
    << ", \"coroutine_size\": " << coroutine_size
    << ", \"services_config\": ";
    for(const auto& srv_conf:server_config){
      os << "{\"name\":"<< srv_conf.name
        << ", \"address\": " << srv_conf.address
        << "}";
    }
    os << ", \"router_file\": " << router_file
    << ", \"monitor_token_bucket\": " << monitor_token_bucket
    << ", \"monitor_queue_bucket\": " << monitor_queue_bucket
    << ", \"monitor_queue_size\": " << monitor_queue_size
    << "}";
}

std::ostream& operator << (std::ostream& os, const ServiceConfig& config) {
  config.Dump(os);
  return os;
}

void ClientRoutersConfig::Dump(std::ostream& os) const {
  os << "{\"type\": \"ClientRoutersConfig\",\"router\":";
  for (const auto& router : client_router_config) {
    os << "[\"name\":" << router.name
      << ", \"mapping\": " << router.mapping
      << ", \"sigle_send_sigle_recv\": " << router.sigle_send_sigle_recv
      << ",\"clients\":";
      for (const auto& cli: router.clients){
        os << "[\"heartbeat\":" << cli.heartbeat
          << "\"timeout\":" << cli.timeout
          << "\"single_addr_connect_count\":" << cli.single_addr_connect_count
          << "\"address\":"<<cli.address;
      
        os << "]";
      }
      os << "]";
  }
  os << "}";
}

std::ostream& operator << (std::ostream& os, const ClientRoutersConfig& config) {
  config.Dump(os);
  return os;
}

ConfigInfo::ConfigInfo() {
}

ConfigInfo::~ConfigInfo() {
}

int ConfigInfo::LoadConfig(const std::string& config_path){
  CIniFileS ini_config;
  if (!ini_config.Open(config_path.c_str())) {
    WARN(logger_, "open config file error,path:" << config_path);
    return -1;
  }

  if (0 != LoadServiceConfig(ini_config, &service_config_)) {
    return -2;
  }
  
  if (0 != LoadServicesConfig(
    ini_config, service_config_.service_count,service_config_.server_config)) {
    return -3;
  }

  if (0!= LoadRouterConfig(service_config_.router_file)){
    return -4;
  }

  if (0 != LoadApplicationConfig(ini_config)){
    return -5;
  }
  
  INFO(logger_, "config info" << *this);
  return 0;
}

int ConfigInfo::LoadServiceConfig(CIniFileS& ini, ServiceConfig* config){
  config->slave_thread_count = ini.GetValueInt("io_service", "slave_thread_count", 2);
  config->service_handle_thread_count = ini.GetValueInt("io_service", "service_handle_thread_count", 2);
  config->service_count = ini.GetValueInt("io_service", "services_count", 1);

  config->router_file = ini.GetValue("io_service", "router_file", "");

  config->coroutine_size = ini.GetValueInt("io_service", "coroutine_size", 512);

  config->monitor_file_name = ini.GetValue("io_service", "monitor_file", "");
  config->monitor_token_bucket = ini.GetValueInt("io_service", "monitor_token_bucket", 16);
  config->monitor_queue_bucket = ini.GetValueInt("io_service", "monitor_queue_bucket", 16);
  config->monitor_queue_size = ini.GetValueInt("io_service", "monitor_queue_size", 1024*512);
  
  return 0;
}

int ConfigInfo::LoadServicesConfig(CIniFileS& ini, int srv_cnt, std::vector<ServerConfig>& config) {
  if (srv_cnt < 1){
    return -1;
  }

  for (int idx = 0; idx < srv_cnt;idx++) {
    ServerConfig srv_config;
    std::string section("service");
    section.append(common::ToString(idx));
    srv_config.name = ini.GetValue(section.c_str(), "name", "");
    srv_config.address = ini.GetValue(section.c_str(), "address", "");
    config.push_back(srv_config);
  }
  
  return 0;
}

int ConfigInfo::LoadRouterConfig(const std::string& file_path){
  if (file_path.empty()){
    INFO(logger_, "not need RouterConfig.");
    return 0;
  }
  CIniFileS ini_config;
  if (!ini_config.Open(file_path.c_str())) {
    WARN(logger_, "open router config file error,path:" << file_path);
    return -1;
  }

  int clients_cnt = ini_config.GetValueInt("router", "client_count", 1);
  for (int idx = 0; idx < clients_cnt;idx++) {
    std::string section("client");
    section.append(common::ToString(idx));
    ClientRouterConfig router;
    router.name  = ini_config.GetValue(section.c_str(), "name", "");
    router.mapping = ini_config.GetValue(section.c_str(), "mapping", "");
    router.sigle_send_sigle_recv = 
      ini_config.GetValueInt(section.c_str(), "sigle_send_sigle_recv", 0);
    int addr_cnt = ini_config.GetValueInt(section.c_str(), "address_count", 1);

    for (int jdx = 0; jdx < addr_cnt; jdx++) {
      ClientConfig client;
      client.single_addr_connect_count =
        ini_config.GetValueInt(section.c_str(), "single_addr_connect_count", 3);
      client.heartbeat = ini_config.GetValueInt(section.c_str(), "heartbeat_ms", 3000);
      client.timeout = ini_config.GetValueInt(section.c_str(), "timeout_ms", 100);
      std::string section1("address");
      section1.append(common::ToString(jdx));
      client.address = ini_config.GetValue(section.c_str(), section1.c_str(), "");
      router.clients.push_back(client);
    }
    client_routers_config_.client_router_config.push_back(router);
  }
  return 0;
}

int ConfigInfo::LoadApplicationConfig(CIniFileS& ini){
  return 0;
}

void ConfigInfo::Dump(std::ostream& os) const{
  os << "ServiceConfig:" << service_config_ << std::endl;
  os << "ClientRouterConfig:" << client_routers_config_ << std::endl;
}

std::ostream& operator << (std::ostream& os, const ConfigInfo& config){
  config.Dump(os);
  return os;
}

}
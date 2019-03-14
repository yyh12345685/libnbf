#include "app/config_info.h"
#include "common/ini_files.h"
#include "common/string_util.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ConfigInfo)

void IoServiceConfig::Dump(std::ostream& os) const {
  os << "{\"type\": \"IoServiceConfig\""
    << ", \"slave_thread_count\": " << slave_thread_count
    << ", \"service_count\": " << service_count
    << ", \"stack_size\": " << stack_size
    << ", \"services_config\": ";
    for(const auto& srv_conf:services_config){
      os << "{\"name\":"<< srv_conf.name
        << ", \"address\": " << srv_conf.address
        << "}";
    }
    os << ", \"monitor_token_bucket\": " << monitor_token_bucket
    << ", \"monitor_queue_bucket\": " << monitor_queue_bucket
    << ", \"monitor_queue_size\": " << monitor_queue_size
    << "}";
}

std::ostream& operator << (std::ostream& os, const IoServiceConfig& config) {
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
    return false;
  }

  if (0 != LoadIoServiceConfig(ini_config, &io_service_config_)) {
    return -1;
  }
  
  if (0 != LoadServicesConfig(
    ini_config, io_service_config_.service_count,io_service_config_.services_config)) {
    return -1;
  }

  if (0 != LoadApplicationConfig(ini_config)) {
    return -1;
  }
  
  TRACE(logger_, "config info" << *this);
  return 0;
}

int ConfigInfo::LoadIoServiceConfig(CIniFileS& ini, IoServiceConfig* config){
  config->slave_thread_count = ini.GetValueInt("io_service", "slave_thread_count", 2);
  config->service_count = ini.GetValueInt("io_service", "services_count", 1);
  config->stack_size = ini.GetValueInt("io_service", "stack_size", 2048);

  config->monitor_token_bucket = ini.GetValueInt("io_service", "monitor_token_bucket", 16);
  config->monitor_queue_bucket = ini.GetValueInt("io_service", "monitor_queue_bucket", 16);
  config->monitor_queue_size = ini.GetValueInt("io_service", "monitor_queue_size", 1024*512);
  
  return 0;
}

int ConfigInfo::LoadServicesConfig(CIniFileS& ini, int srv_cnt, std::vector<ServiceConfig>& config) {
  if (srv_cnt < 1){
    return -1;
  }

  for (int idx = 0; idx < srv_cnt;idx++) {
    ServiceConfig srv_config;
    std::string section("service");
    section.append(common::ToString(idx));
    srv_config.name = ini.GetValue(section.c_str(), "name", "");
    srv_config.address = ini.GetValue(section.c_str(), "address", "");
    config.push_back(srv_config);
  }
  
  return 0;
}

int ConfigInfo::LoadApplicationConfig(CIniFileS& ini){
  return 0;
}

void ConfigInfo::Dump(std::ostream& os) const{
  os << "IOServiceConfig:" << io_service_config_ << std::endl;
}

std::ostream& operator << (std::ostream& os, const ConfigInfo& config){
  config.Dump(os);
  return os;
}

}
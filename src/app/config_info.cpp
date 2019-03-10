#include "app/config_info.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ConfigInfo)

ConfigInfo::ConfigInfo() {
}

ConfigInfo::~ConfigInfo() {
}

int ConfigInfo::LoadConfig(const std::string& config_path){
  return 0;
}

int ConfigInfo::LoadIOServiceConfig(CIniFileS& ini, IOServiceConfig* config){
  return 0;
}

int ConfigInfo::LoadApplicationConfig(CIniFileS& ini){
  return 0;
}

int ConfigInfo::LoadServicesConfig(CIniFileS& ini, ServicesConfig* config){
  return 0;
}

int ConfigInfo::LoadClientRoutersConfig(CIniFileS& ini, ClientRoutersConfig* config){
  return 0;
}

}
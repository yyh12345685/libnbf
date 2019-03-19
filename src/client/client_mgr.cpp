#include "client/client_mgr.h"
#include "app/config_info.h"
#include "client/client_routers.h"
#include "message.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ClientMgr)

ClientMgr::ClientMgr(){
}

ClientMgr::~ClientMgr() {
  Stop();
  for (const auto& it : router_maps_){
    delete it.second;
  }
}

int ClientMgr::Start(const ClientRoutersConfig& routers_config){
  if (0 == routers_config.client_router_config.size()){
    DEBUG(logger_, "no ClientRouters,not need client in config.");
    return 0;
  }
  for (const auto& rt_cfg:routers_config.client_router_config){
    if (router_maps_.find(rt_cfg.name) != router_maps_.end()){
      WARN(logger_, "ClientMgr::Start Dupldate client:" << rt_cfg.name);
      continue;
    }
    ClientRouters* routers = new ClientRouters(rt_cfg.name, rt_cfg.mapping);
    if (0!= routers->Start(rt_cfg.clients)){
      delete routers;
      return -1;
    }
    router_maps_[rt_cfg.name] = routers;
  }
  return 0;
}

int ClientMgr::Stop(){
  for (auto& router : router_maps_) {
    router.second->Stop();
  }
  return 0;
}

bool ClientMgr::Send(const std::string& router, EventMessage * message){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers){
    return false;
  }
  return routers->Send(message);
}
bool ClientMgr::SendHash(const std::string& router, EventMessage* message, uint32_t hash){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    return false;
  }
  return routers->SendHash(message,hash);
}

EventMessage* ClientMgr::SendRecieve(
  const std::string& router,
  EventMessage* message,
  uint32_t timeout_ms) {
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    return nullptr;
  }
  return routers->SendRecieve(message, timeout_ms);
}
EventMessage* ClientMgr::SendRecieveHash(
  const std::string& router,
  EventMessage* message,
  uint32_t hash,
  uint32_t timeout_ms) {
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    return nullptr;
  }
  return routers->SendRecieveHash(message, hash, timeout_ms);
}

ClientRouters* ClientMgr::GetClientRouters(const std::string& router){
  auto it = router_maps_.find(router);
  if (it != router_maps_.end()) {
    return it->second;
  }
  return nullptr;
}

}

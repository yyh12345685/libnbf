#include "client/client_mgr.h"
#include "app/config_info.h"
#include "client/client_routers.h"
#include "net/client_reconnect_thread.h"
#include "message.h"
#include "monitor/mem_profile.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ClientMgr)

ClientMgr::ClientMgr(){
}

ClientMgr::~ClientMgr() {
  Stop();
  for (const auto& it : router_maps_){
    BDF_DELETE (it.second);
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
    ClientRouters* routers = BDF_NEW (
      ClientRouters,rt_cfg.name, rt_cfg.mapping, rt_cfg.sigle_send_sigle_recv);

    if (0!= routers->Start(rt_cfg.clients)){
      BDF_DELETE (routers);
      return -1;
    }
    router_maps_[rt_cfg.name] = routers;
  }

  ClientReconnect::GetInstance().StartThread();
  return 0;
}

int ClientMgr::Stop(){
  ClientReconnect::GetInstance().StopThread();

  for (auto& router : router_maps_) {
    router.second->Stop();
  }
  return 0;
}

//单项发送，不关心是否返回
bool ClientMgr::Send(const std::string& router, EventMessage * message){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers){
    MessageFactory::Destroy(message);
    return false;
  }
  return routers->Send(message);
}

bool ClientMgr::SendHash(const std::string& router, EventMessage* message, uint32_t hash){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    MessageFactory::Destroy(message);
    return false;
  }
  return routers->SendHash(message,hash);
}

bool ClientMgr::Invoke(
  const std::string& router,
  EventMessage* message,
  const InvokerCallback& cb){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }
  return routers->Invoke(message,cb);
}

bool ClientMgr::Invoke(
  const std::string& router,
  EventMessage* message,
  const InvokerCallback& cb,
  uint32_t hash){
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }
  return routers->Invoke(message,cb, hash);
}

//支持协程
EventMessage* ClientMgr::SendRecieve(
  const std::string& router,
  EventMessage* message,
  uint32_t timeout_ms) {
  ClientRouters* routers = GetClientRouters(router);
  if (nullptr == routers) {
    MessageFactory::Destroy(message);
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
    MessageFactory::Destroy(message);
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

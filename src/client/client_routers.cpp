#include "client/client_routers.h"
#include "client/client_router.h"
#include "app/config_info.h"
#include "message.h"
#include "monitor/mem_profile.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ClientRouters)

ClientRouters::ClientRouters(
  const std::string& name, 
  const std::string& mapping,
  const bool& sigle_send_sigle_recv):
  name_(name),
  mapping_(mapping),
  current_(0),
  sigle_send_sigle_recv_(sigle_send_sigle_recv){
}

ClientRouters::~ClientRouters() {
  Stop();
  for (const auto& router:client_routers_){
    BDF_DELETE(router);
  }
}

int ClientRouters::Start(const std::vector<ClientConfig>& clients) {
  for (const auto& cli:clients){
    ClientRouter* router = BDF_NEW(ClientRouter,name_,sigle_send_sigle_recv_);
    if (0!= router->Start(cli)){
      BDF_DELETE(router);
      return -1;
    }
    client_routers_.push_back(router);
  }
  return 0;
}

int ClientRouters::Stop() {
  for (const auto& router : client_routers_) {
    router->Stop();
  }
  return 0;
}

bool ClientRouters::Send(EventMessage * message) {
  int idx = current_++;
  ClientRouter* router = client_routers_[idx%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return false;
  }
  return router->Send(message);
}

bool ClientRouters::SendHash(EventMessage* message, uint32_t hash){
  ClientRouter* router = client_routers_[hash%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return false;
  }
  return router->Send(message);
}

bool ClientRouters::Invoke(EventMessage * message, const InvokerCallback& cb){
  int idx = current_++;
  ClientRouter* router = client_routers_[idx%client_routers_.size()];
  if (!router) {
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }
  return router->Invoke(message,cb);
}

bool ClientRouters::Invoke(EventMessage* message, const InvokerCallback& cb, uint32_t hash){
  ClientRouter* router = client_routers_[hash%client_routers_.size()];
  if (!router) {
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }
  return router->Invoke(message,cb);
}

EventMessage* ClientRouters::SendRecieve(EventMessage* message) {
  uint32_t idx = current_++;
  ClientRouter* router = client_routers_[idx%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return nullptr;
  }
  return router->SendRecieve(message);
}

EventMessage* ClientRouters::SendRecieveHash(
  EventMessage* message, uint32_t hash) {
  ClientRouter* router = client_routers_[hash%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return nullptr;
  }
  return router->SendRecieve(message);
}

}

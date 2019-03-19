#include "client/client_routers.h"
#include "client/client_router.h"
#include "app/config_info.h"
#include "message.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ClientRouters)

ClientRouters::ClientRouters(
  const std::string& name, const std::string& mapping):
  name_(name),
  mapping_(mapping),
  current_(0){
}

ClientRouters::~ClientRouters() {
  Stop();
  for (const auto& router:client_routers_){
    delete router;
  }
}

int ClientRouters::Start(const std::vector<ClientConfig>& clients) {
  for (const auto& cli:clients){
    ClientRouter* router = new ClientRouter(name_);
    if (0!= router->Start(cli)){
      delete router;
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

EventMessage* ClientRouters::SendRecieve(EventMessage* message, uint32_t timeout_ms) {
  uint32_t idx = current_++;
  ClientRouter* router = client_routers_[idx%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return nullptr;
  }
  return router->SendRecieve(message, timeout_ms);
}

EventMessage* ClientRouters::SendRecieveHash(
  EventMessage* message, uint32_t hash, uint32_t timeout_ms) {
  ClientRouter* router = client_routers_[hash%client_routers_.size()];
  if (!router) {
    MessageFactory::Destroy(message);
    return nullptr;
  }
  return router->SendRecieve(message, timeout_ms);
}

}

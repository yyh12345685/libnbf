#include "client/client_router.h"
#include "client/client.h"
#include "app/config_info.h"
#include "monitor/matrix_scope.h"
#include "monitor/mem_profile.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ClientRouter)

ClientRouter::ClientRouter(const std::string& name):
  name_(name),
  current_(0){
}

ClientRouter::~ClientRouter(){
  for (const auto& cli:clients_){
    BDF_DELETE(cli);
  }
}

int ClientRouter::Start(const ClientConfig& cli_config){
  for (int idx = 0; idx < cli_config.single_addr_connect_count;idx++) {
    Client* client = BDF_NEW(Client,
      name_, cli_config.address, cli_config.timeout, cli_config.heartbeat);
    if (0!= client->Start()){
      BDF_DELETE (client);
      return -1;
    }
    clients_.push_back(client);
  }
  return 0;
}

int ClientRouter::Stop(){
  for (const auto& cli : clients_) {
    cli->Stop();
  }
  return 0;
}

bool ClientRouter::Send(EventMessage * message){
  int idx = current_++;
  Client* cli = clients_[idx%clients_.size()];
  if (!cli) {
    MessageFactory::Destroy(message);
    return false;
  }
  return cli->Send(message);
}

EventMessage* ClientRouter::DoSendRecieve(EventMessage* message, uint32_t timeout_ms){
  monitor::MatrixScope matrix_scope(name_, monitor::MatrixScope::kModeAutoFail);
  uint32_t idx = current_++;
  Client* cli = clients_[idx%clients_.size()];
  if (!cli) {
    DEBUG(logger_, "ClientRouter::DoSendRecieve no avaliable client:" << name_);
    MessageFactory::Destroy(message);
    return nullptr;
  }

  EventMessage* response = cli->SendRecieve(message, timeout_ms);
  if (response) {
    matrix_scope.SetOkay(true);
  }
  return response;
}

}

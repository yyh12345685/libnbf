#include "client/client_router.h"
#include "client/client.h"
#include "app/config_info.h"
#include "monitor/matrix_scope.h"
#include "monitor/mem_profile.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, ClientRouter)

ClientRouter::ClientRouter(const std::string& name, const bool& sigle_send_sigle_recv):
  name_(name),
  current_(0),
  sigle_send_sigle_recv_(sigle_send_sigle_recv){
}

ClientRouter::~ClientRouter(){
  for (const auto& cli:clients_){
    BDF_DELETE(cli);
  }
}

int ClientRouter::Start(const ClientConfig& cli_config){
  for (int idx = 0; idx < cli_config.single_addr_connect_count;idx++) {
    Client* client = BDF_NEW(Client,
      cli_config.address, cli_config.timeout, cli_config.heartbeat,sigle_send_sigle_recv_);
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

Client* ClientRouter::GetValidClient() {
  size_t loop_cnt = 0;
  while ((loop_cnt++) < clients_.size()){
    int idx = current_++;
    Client* cli = clients_[idx%clients_.size()];
    if (cli && cli->GetClientStatus() == Client::kWorking && cli->TrySetBusy()){
      return cli;
    }
  }
  INFO(logger_, "no valid client,name:" << name_<<",loop times:"<< loop_cnt);
  return nullptr;
}

bool ClientRouter::Send(EventMessage * message){
  monitor::MatrixScope matrix_scope(name_, monitor::MatrixScope::kModeAutoSuccess);
  Client* cli = GetValidClient();
  //当sigle_send_sigle_recv=1的时候,连接数需要比较多，如果获取不到cli，需重新send
  if (!cli) {
    matrix_scope.SetOkay(false);
    MessageFactory::Destroy(message);
    return false;
  }
  return cli->Send(message);
}

bool ClientRouter::Invoke(EventMessage * message, const InvokerCallback& cb){
  Client* cli = GetValidClient();
  //当sigle_send_sigle_recv=1的时候,连接数需要比较多，如果获取不到cli，需重新send
  if (!cli) {
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }
  return cli->Invoke(message,cb,name_);
}

EventMessage* ClientRouter::DoSendRecieve(EventMessage* message, uint32_t timeout_ms){
  monitor::MatrixScope matrix_scope(name_, monitor::MatrixScope::kModeAutoFail);
  Client* cli = GetValidClient();
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

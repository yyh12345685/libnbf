#include "client/client_router.h"
#include "client/client.h"

namespace bdf{

ClientRouter::ClientRouter(const std::string& name) {
}

ClientRouter::~ClientRouter(){

}

int ClientRouter::AddClient(const std::string& address, uint32_t timeout_ms) {
  return 0;
}

int ClientRouter::Start(){
  return 0;
}

int ClientRouter::Stop(){
  return 0;
}

bool ClientRouter::Send(EventMessage * message){
  return 0;
}

EventMessage* ClientRouter::DoSendRecieve(EventMessage* message, uint32_t timeout_ms){
  return nullptr;
}

Client* ClientRouter::GetNextClient(){
  return nullptr;
}

Client* ClientRouter::GetHashClient(uint32_t hash){
  return nullptr;
}

}

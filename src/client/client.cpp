#include "client/client.h"
#include "service/io_service.h"
#include "app/config_info.h"
#include "protocol/protocol_helper.h"
#include "net/async_client_connect.h"
#include "net/sync_client_connect.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, Client)

Client::Client(
  const std::string& name,
  const std::string& address,
  uint32_t timeout_ms,
  uint32_t heartbeat_ms): 
  name_(name), 
  address_(address),
  timeout_ms_(timeout_ms), 
  heartbeat_ms_(heartbeat_ms), 
  connect_(NULL){
}

Client::~Client(){
  Stop();
}

int Client::Start() {
  ClientConnect* con = CreateClient(address_, timeout_ms_, heartbeat_ms_);
  if (!con) {
    ERROR(logger_, "Client::Start CreateClient fail"<< ", address:" << address_);
    return -1;
  }

  if (0 != con->TryConnect()) {
    ERROR(logger_, "Client::Start fail"<< ", address:" << address_);
    delete con;
    return -2;
  }

  connect_ = con;
  return 0;
}

ClientConnect* Client::CreateClient(
  const std::string& address, uint32_t timeout_ms, uint32_t heartbeat_ms){
  char ip[1024];
  int port;
  int protocol_type = ProtocolHelper::ParseSpecAddr(address.c_str(),ip,&port);
  if (protocol_type == MessageType::kUnknownEvent){
    WARN(logger_, "ParseSpecAddr faild,address:" << address);
    return nullptr;
  }

  ProtocolFactory protocol_factory;
  ProtocolBase* protocol = protocol_factory.Create(protocol_type);

  ClientConnect* client_connect = NULL;

  if (protocol->IsSynchronous()) {
    client_connect = new SyncClientConnect(timeout_ms, heartbeat_ms);
  } else {
    client_connect = new AsyncClientConnect(timeout_ms, heartbeat_ms);
  }

  client_connect->SetIp(ip);
  client_connect->SetPort(port);
  client_connect->SetProtocol(protocol);
  return client_connect;
}

int Client::Stop() {
  if (!connect_) {
    return 0;
  }

  int ret = connect_->Stop();
  //may be a bug
  //connect_->Destroy();
  connect_ = NULL;
  return ret;
}

int64_t Client::GetSequenceId(){
  static std::atomic<int64_t> sequence_id_(0);

  int64_t ret = sequence_id_++;
  if (ret < 0){
    sequence_id_.store(0);
  }
  return ret;
}

bool Client::Send(EventMessage* message) {
  if (GetClientStatus() != kWorking) {
    //DEBUG(logger, "Client::Send Channel Broken" << *this);
    MessageFactory::Destroy(message);
    return false;
  }

  message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  message->direction = MessageBase::kOneway;
  IoService::GetInstance().SendToIoHandle(message);
  return true;
}

EventMessage* Client::DoSendRecieve(EventMessage* message, uint32_t timeout_ms) {
  if (GetClientStatus() != kWorking) {
    //DEBUG(logger, "Client::Send Channel Broken" << *this);
    MessageFactory::Destroy(message);
    return nullptr;
  }

  message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  message->direction = MessageBase::kOutgoingRequest;
  IoService::GetInstance().SendToIoHandle(message);
  //TODO...
  return nullptr;
}

}

#include "client/client.h"
#include "service/io_service.h"
#include "app/config_info.h"
#include "protocol/protocol_helper.h"
#include "net/async_client_connect.h"
#include "net/sync_client_connect.h"
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_context.h"
#include "service/coroutine_service_handle.h"
#include "client/client_mgr.h"
#include "net/connect_manager.h"
#include "monitor/matrix.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, Client)

Client::Client(
  const std::string& address,
  uint32_t timeout_ms,
  uint32_t heartbeat_ms,
  const bool& sigle_send_sigle_recv):
  address_(address),
  timeout_ms_(timeout_ms), 
  heartbeat_ms_(heartbeat_ms), 
  connect_(nullptr),
  sigle_send_sigle_recv_(sigle_send_sigle_recv){
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

  if (0 != con->TryConnect()){
    ERROR(logger_, "Client::Start fail"<< ", address:" << address_);
    BDF_DELETE(con);
    return -2;
  }

  connect_ = con;
  ConnectManager::Instance().RegisterConnect((uint64_t)con, con);
  return 0;
}

ClientConnect* Client::CreateClient(
  const std::string& address, uint32_t timeout_ms, uint32_t heartbeat_ms){
  char ip[256];
  int port;
  int protocol_type = ProtocolHelper::ParseSpecAddr(address.c_str(),ip,&port);
  if (protocol_type == MessageType::kUnknownEvent){
    WARN(logger_, "ParseSpecAddr faild,address:" << address);
    return nullptr;
  }

  ProtocolFactory protocol_factory;
  ProtocolBase* protocol = protocol_factory.Create(protocol_type);

  ClientConnect* client_connect = nullptr;

  if (protocol->IsSynchronous()) {
    client_connect = 
      BDF_NEW (SyncClientConnect,timeout_ms, heartbeat_ms,sigle_send_sigle_recv_);
  } else {
    client_connect = BDF_NEW (AsyncClientConnect,timeout_ms, heartbeat_ms);
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

  ConnectManager::Instance().UnRegisterConnect((uint64_t)connect_);
  int ret = connect_->Stop();
  //may be a bug
  //connect_->Destroy();
  connect_ = nullptr;
  return ret;
}

void Client::Dump(std::ostream& os) const {
  os << "{\"type\": \"Client\""
    << ", \"address\": \"" << address_ << "\""
    << ", \"timeout\": " << timeout_ms_
    << ", \"heartbeat\": " << heartbeat_ms_
    << ", \"status\": \"" << connect_->GetStatus() << "\""
    << "}" << std::endl;
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
    INFO(logger_, "Client::Send Channel Broken" << *this);
    MessageFactory::Destroy(message);
    SetNoBuzy();
    return false;
  }

  //message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  if (message->IsSynchronous()){
    message->direction = MessageBase::kSendNoCareResponse;
  }else{
    message->direction = MessageBase::kOnlySend;
  }
  
  DoSend(message);
  return true;
}

bool Client::Invoke(EventMessage* message, const InvokerCallback& cb, const std::string& name){
  if (GetClientStatus() != kWorking) {
    INFO(logger_, "Client::Send Channel Broken" << *this);
    message->status = MessageBase::kInvokeError;
    cb(message);
    SetNoBuzy();
    return false;
  }

  message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  message->direction = MessageBase::kOutgoingRequest;
  message->ctx = BDF_NEW(ContextBase);
  message->ctx->callback = cb;
  message->ctx->monitor_id = monitor::GlobalMatrix::Instance().MarkBegin(name);
  DoSend(message);
  return true;
}

EventMessage* Client::DoSendRecieve(EventMessage* message, uint32_t timeout_ms) {
  if (GetClientStatus() != kWorking) {
    INFO(logger_, "Client::DoSendRecieve Channel Broken" << *this);
    MessageFactory::Destroy(message);
    SetNoBuzy();
    return nullptr;
  }

  message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  message->direction = MessageBase::kOutgoingRequest;
  if (ForTest::Inst().GetForTest()){
    TRACE(logger_, "for test_client_server,only send.");
    if (message->IsSynchronous()) {
      message->direction = MessageBase::kSendNoCareResponse;
    } else {
      message->direction = MessageBase::kOnlySend;
    }
    DoSend(message);
    return nullptr;
  }
  if (nullptr != CoroutineContext::Instance().GetCoroutine()){
    CoroutineContext::Instance().GetCoroutine()->SetWaitingId(message->sequence_id);
  }
  
  DoSend(message);
  //TRACE(logger_, "send msg handle_id:" << message->handle_id);
  EventMessage* response = DoRecieve(timeout_ms);
  if (response && response->status != EventMessage::kStatusOK) {
    TRACE(logger_, "may be time out,status:" << response->status << ",msg:" << *response);
    MessageFactory::Destroy(response);
    return nullptr;
  }
  return response;
}

void Client::DoSend(EventMessage* message){
  if (nullptr != CoroutineContext::Instance().GetServiceHandler()){
    //使用协程handle的时候不为空，非协程框架暂未完整实现
    message->handle_id = CoroutineContext::Instance().GetServiceHandler()->GetHandlerId();
  }
  
  TRACE(logger_, "SendToIoHandle,handle_id:" << message->handle_id);
  IoService::GetInstance().SendToIoHandle(message);
}

EventMessage* Client::DoRecieve(uint32_t timeout_ms){
  if (nullptr != CoroutineContext::Instance().GetCoroutine()){
    return CoroutineContext::Instance().GetCoroutine()->RecieveMessage(timeout_ms);
  }else{
    //非协程仅供测试使用，非协程需要实现异步调用，用回调函数处理
    return nullptr;
  }
  
}

std::ostream& operator << (std::ostream& os, const Client& client) {
  client.Dump(os);
  return os;
}

}

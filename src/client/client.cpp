#include "client/client.h"
#include "service/service_manager.h"
#include "app/config_info.h"
#include "protocol/protocol_helper.h"
#include "net/async_client_connect.h"
#include "net/sync_client_connect.h"
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_manager.h"
#include "coroutine/coro_context.h"
#include "service/coroutine_service_handle.h"
#include "client/client_mgr.h"
#include "net/connect_manager.h"
#include "monitor/matrix.h"
#include "monitor/matrix_scope.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_threads.h"
#include "common/rate_limiter.h"
#include "common/time.h"

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
  inflight_(std::make_shared<std::atomic<int64_t> >(0)),
  sigle_send_sigle_recv_(sigle_send_sigle_recv){
}

Client::~Client(){
  Stop();
}

int Client::Start() {
  std::lock_guard<std::mutex> guard(connect_lock_);
  if (connect_) return 0;
  ClientConnect* con = CreateClient(address_, heartbeat_ms_);
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
  synchronous_ = con->GetProtocol() && con->GetProtocol()->IsSynchronous();
  //ConnectManager::Instance().RegisterConnect((uint64_t)con, con);
  return 0;
}

ClientConnect* Client::CreateClient(const std::string& address, uint32_t heartbeat_ms){
  char ip[256];
  int port;
  int protocol_type = ProtocolHelper::ParseSpecAddr(address.c_str(),ip,&port);
  if (protocol_type == MessageType::kUnknownEvent){
    WARN(logger_, "ParseSpecAddr faild,address:" << address);
    return nullptr;
  }

  static std::atomic<int64_t> connect_id(0);

  ProtocolFactory protocol_factory;
  ProtocolBase* protocol = protocol_factory.Create(protocol_type);

  ClientConnect* client_connect = nullptr;
  if (protocol->IsSynchronous()) {
    client_connect = 
      BDF_NEW (SyncClientConnect,timeout_ms_, heartbeat_ms,sigle_send_sigle_recv_);
  } else {
    client_connect = BDF_NEW (AsyncClientConnect,timeout_ms_, heartbeat_ms);
  }

  client_connect->SetIp(ip);
  client_connect->SetPort(port);
  client_connect->SetProtocol(protocol);
  client_connect->SetConnectId(connect_id);
  connect_id++;
  return client_connect;
}

int Client::Stop() {
  std::lock_guard<std::mutex> guard(connect_lock_);
  ClientConnect* con = connect_;
  connect_ = nullptr;
  if (!con) {
    return 0;
  }
  return con->Stop();
}

void Client::Dump(std::ostream& os) const {
  std::lock_guard<std::mutex> guard(connect_lock_);
  os << "{\"type\": \"Client\""
    << ", \"address\": \"" << address_ << "\""
    << ", \"timeout\": " << timeout_ms_
    << ", \"heartbeat\": " << heartbeat_ms_
    << ", \"status\": \"" << (connect_ ? connect_->GetStatus() : -1) << "\""
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

uint32_t Client::InflightLimit() const {
  const uint32_t limit = service::GetServiceManager().GetServiceConfig().client_max_inflight;
  if (synchronous_ && (0 == limit || limit > kMaxSyncInflight)) {
    return kMaxSyncInflight;
  }
  return limit;
}

bool Client::InflightAtLimit() const {
  const uint32_t limit = InflightLimit();
  return 0 != limit
    && inflight_->load(std::memory_order_relaxed) >= static_cast<int64_t>(limit);
}

bool Client::QueueAtLimit(size_t* pending) const {
  const uint32_t limit = service::GetServiceManager().GetServiceConfig().client_send_queue_limit;
  NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
  if (0 == limit || nullptr == net) {
    return false;
  }
  // Same pointer value PutMessageToHandle hashes (descriptor_id), so this is
  // the queue the message would go to.
  Connecting* con = Connecting::DescriptorConnect((int64_t)((void*)connect_));
  *pending = net->GetIoThreads()->PendingSize(con);
  return *pending >= limit;
}

bool Client::HasCapacity(bool for_invoke) const {
  std::lock_guard<std::mutex> guard(connect_lock_);
  if (!connect_ || connect_->GetStatus() != ClientConnect::kConnected) {
    return false;
  }
  size_t pending = 0;
  return !(for_invoke && InflightAtLimit()) && !QueueAtLimit(&pending);
}

bool Client::SendQueueFull() {
  size_t pending = 0;
  if (!QueueAtLimit(&pending)) {
    return false;
  }
  const uint32_t limit = service::GetServiceManager().GetServiceConfig().client_send_queue_limit;
  monitor::MatrixCounter::Add("client_send_queue_full", 1);
  static common::RateLimiter full_log;
  uint64_t suppressed = 0;
  if (full_log.Allow(&suppressed)) {
    WARN(logger_, "client send queue full, reject, pending:" << pending << ",limit:" << limit
      << ",address:" << address_ << ",suppressed since last log:" << suppressed);
  }
  return true;
}

bool Client::InflightFull() {
  if (!InflightAtLimit()) {
    return false;
  }
  monitor::MatrixCounter::Add("client_inflight_full", 1);
  static common::RateLimiter full_log;
  uint64_t suppressed = 0;
  if (full_log.Allow(&suppressed)) {
    WARN(logger_, "client in-flight limit reached, reject invoke, inflight:"
      << inflight_->load(std::memory_order_relaxed) << ",limit:" << InflightLimit()
      << ",address:" << address_ << ",suppressed since last log:" << suppressed);
  }
  return true;
}

void Client::LogBroken(const char* where) {
  // Called without connect_lock_ (Dump takes it). While a backend is down
  // every call fails here, so log at most once per second.
  static common::RateLimiter broken_log;
  uint64_t suppressed = 0;
  if (broken_log.Allow(&suppressed)) {
    INFO(logger_, where << " Channel Broken" << *this
      << ",suppressed since last log:" << suppressed);
  }
}

// Called with connect_lock_ held. Returns false (and releases the busy flag)
// when the request cannot be queued: not connected, or the I/O backlog is
// at client_send_queue_limit.
static bool CanQueue(ClientConnect* con, bool queue_full) {
  if (con && con->GetStatus() == ClientConnect::kConnected && !queue_full) {
    return true;
  }
  if (con) con->SetBuzy(false);
  return false;
}

bool Client::Send(EventMessage* message) {
  std::unique_lock<std::mutex> guard(connect_lock_);
  const bool connected = connect_ && connect_->GetStatus() == ClientConnect::kConnected;
  if (!CanQueue(connect_, connected && SendQueueFull())) {
    guard.unlock();
    if (!connected) LogBroken("Client::Send");
    MessageFactory::Destroy(message);
    return false;
  }

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
  std::unique_lock<std::mutex> guard(connect_lock_);
  const bool connected = connect_ && connect_->GetStatus() == ClientConnect::kConnected;
  if (!CanQueue(connect_, connected && (InflightFull() || SendQueueFull()))) {
    guard.unlock();
    if (!connected) LogBroken("Client::Invoke");
    message->status = MessageBase::kInvokeError;
    cb(message);
    return false;
  }

  message->sequence_id = GetSequenceId();
  message->descriptor_id = (int64_t)((void*)connect_);
  message->direction = MessageBase::kOutgoingRequest;
  message->ctx = BDF_NEW(ContextBase);
  message->ctx->callback = cb;
  message->ctx->monitor_id = monitor::GlobalMatrix::Instance().MarkBegin(name);
  // Released in ~ContextBase, i.e. when the response message is destroyed.
  inflight_->fetch_add(1, std::memory_order_relaxed);
  message->ctx->inflight = inflight_;
  DoSend(message);
  return true;
}

EventMessage* Client::DoSendRecieve(EventMessage* message) {
  std::unique_lock<std::mutex> guard(connect_lock_);
  const bool connected = connect_ && connect_->GetStatus() == ClientConnect::kConnected;
  if (!CanQueue(connect_, connected && SendQueueFull())) {
    guard.unlock();
    if (!connected) LogBroken("Client::DoSendRecieve");
    MessageFactory::Destroy(message);
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
  
  // I/O owns message after enqueue and may destroy it before this thread resumes.
  const uint64_t sequence_id = message->sequence_id;
  DoSend(message);
  guard.unlock();
  EventMessage* response = DoRecieve(sequence_id);
  if (response && response->status != EventMessage::kStatusOK) {
    TRACE(logger_, "may be time out,status:" << response->status << ",msg:" << *response);
    MessageFactory::Destroy(response);
    return nullptr;
  }
  return response;
}

void Client::DoSend(EventMessage* message){
  if (nullptr != CoroutineManager::GetServiceHandler()){
    //使用协程handle的时候不为空，非协程框架使用异步调用Invoke
    message->handle_id = CoroutineManager::GetServiceHandler()->GetHandlerId();
    message->msg_coro = CoroutineManager::GetCurrentCoroutine();
    message->handle_svr = CoroutineManager::GetServiceHandler();
  }
  
  message->birthtime = Time::GetCurrentClockTime();

  TRACE(logger_, "SendToNetThread,handle_id:" << message->handle_id);
  service::GetServiceManager().SendToNetThread(message);
}

EventMessage* Client::DoRecieve(uint64_t sequence_id) {
  CoroContext* coro = CoroutineManager::GetCurrentCoroutine();
  if (!coro || !coro->actor) {
    WARN(logger_, "SendRecieve requires a running coroutine");
    return nullptr;
  }

  coro->actor->SetWaitingId(sequence_id);
  return coro->actor->RecieveMessage(sequence_id, timeout_ms_);
}

std::ostream& operator << (std::ostream& os, const Client& client) {
  client.Dump(os);
  return os;
}

}

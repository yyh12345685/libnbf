#include <sys/epoll.h>
#include "net/client_connect.h"
#include "common/time.h"
#include "service/service_manager.h"
#include "net/socket.h"
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_threads.h"

namespace bdf{

//void HeartBeatTimer::OnTimer(void* function_data){
//  ClientConnect* cli_con = (ClientConnect *)(function_data);
//  cli_con->OnHeartBeat();
//}

LOGGER_CLASS_IMPL(logger, ClientConnect);

ClientConnect::ClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  is_buzy_(false),
  status_(kBroken),
  timeout_ms_(timeout_ms),
  heartbeat_ms_(heartbeat_ms),
  reconnect_timer_(0),
  heartbeat_timer_(0),
  register_thread_id_(-1){
}

ClientConnect::~ClientConnect(){
}

int ClientConnect::TimerThreadId(){
  // Known before the fd is registered, so timers can be armed early.
  return service::GetServiceManager().GetNetThreadManager()
    ->GetIoThreads()->GetIoThreadId(this);
}

// Runs on this connection's I/O loop.
void ClientConnect::OnTimer(void* function_data) {
  uint32_t client_timer_type = *((uint32_t*)(function_data));
  TRACE(logger_, "client_timer_type:"<< client_timer_type);
  if (GetStatus() == kStopping) {
    return;
  }
  switch (client_timer_type){
  case kClientTimerTypeReconnect:
    reconnect_timer_ = 0;
    if (GetStatus() == kConnecting) {
      // Doubles as the connect timeout: still not connected, start over.
      INFO(logger_, "connect timeout,ip:" << GetIp() << ",port:" << GetPort());
      NextAddr();
      OnClose();
    } else if (GetStatus() == kBroken) {
      TryConnect();
    }
    break;
  case kClientTimerTypeHeartBeat:
    heartbeat_timer_ = 0;
    OnHeartBeat();
    break;
  default:
    WARN(logger, "unkown type:" << client_timer_type);
    break;
  }
}

// Initial call: from Client::Start before the connection is published (no
// other thread can touch it yet). Later calls: from the reconnect timer on
// the owning I/O loop. A reconnect never blocks that loop.
int ClientConnect::TryConnect(){
  if (GetStatus() == kStopping) {
    return -1;
  }
  const bool initial = register_thread_id_ < 0;
  if (initial || addrs_.empty()) {
    // Only the first call may block on DNS (it runs on the starting thread).
    if (!initial || !Socket::ResolveAll(GetIp().c_str(), GetPort(), addrs_)) {
      WARN(logger_, "resolve failed,ip:" << GetIp() << ",port:" << GetPort());
      SetStatus(kBroken);
      if (!initial)
        StartReconnectTimer();
      return -1;
    }
    addr_index_ = 0;
  }

  SetStatus(kConnecting);
  const Socket::ResolvedAddr& target = addrs_[addr_index_];
  std::pair<int, bool> connet_info = Socket::Connect(
    reinterpret_cast<const sockaddr*>(&target.addr), target.len);
  if (connet_info.first < 0) {
    WARN(logger_, "connect failed,ip:" << GetIp() << ",port:" << GetPort() << ",errno:" << errno);
    SetFd(-1);
    SetStatus(kBroken);
    NextAddr();
    if (!initial)
      StartReconnectTimer();
    return -1;
  }
  const int fd = connet_info.first;
  SetFd(fd);
  // Do not look at errno: after an immediate success it holds a stale value.
  bool pending = !connet_info.second;
  if (pending && initial && IsConnected(20, 9)) {
    // Startup only: callers expect a usable client right after Start.
    pending = false;
  }
  SetStatus(pending ? kConnecting : kConnected);

  // Arm the timer before the fd becomes visible to the I/O loop, so nothing
  // on that loop can run concurrently with this setup.
  register_thread_id_ = TimerThreadId();
  if (pending) {
    StartReconnectTimer(kConnectTimeoutMs); // connect timeout; OnConnectWrite cancels it
  } else {
    StartHeartBeatTimer();
  }

  // Pending: also wait for EPOLLOUT, delivered to OnConnectWrite.
  int ret = pending ? RegisterAddModrw(fd, register_thread_id_)
                    : RegisterAddModr(fd, register_thread_id_);
  if (0 != ret) {
    WARN(logger_, "register failed,fd:" << fd);
    SetStatus(kBroken);
    CancelTimer();
    Socket::Close(fd); // IoThreads already undid a partial registration
    SetFd(-1);         // so a later Clean() cannot close a reused fd number
    NextAddr();
    if (!initial)
      StartReconnectTimer();
    return -2;
  }
  return 0;
}

int ClientConnect::StartReconnectTimer(uint32_t delay_ms) {
  TRACE(logger_, "ClientConnect::StartReconnectTimer:"<<this);
  if (GetStatus() == kStopping) {
    return 0;
  }
  if (0 != reconnect_timer_) {
    WARN(logger, "ClientChannel::StartReconnectTimer duplicate!");
    return -1;
  }
  TimerData td;
  td.time_out_ms = delay_ms ? delay_ms : kReconnectIntervalMs;
  td.time_proc = this;
  td.function_data = (void*)(&client_timer_type_reconnect_);
  reconnect_timer_ = service::GetServiceManager().GetNetThreadManager()
    ->StartTimer(td, TimerThreadId());
  return 0;
}

int ClientConnect::StartHeartBeatTimer() {
  TRACE(logger_, "ClientConnect::StartHeartBeatTimer...");
  if (0 == heartbeat_ms_ || GetStatus() != kConnected || 0 != heartbeat_timer_) {
    return 0;
  }

  TimerData td;
  td.time_out_ms = heartbeat_ms_;
  td.time_proc = this;
  td.function_data = (void*)(&client_timer_type_heartbeat_);
  // Keep the id: it used to be dropped, so heartbeats could never be
  // cancelled and kept firing after close/delete.
  heartbeat_timer_ = service::GetServiceManager().GetNetThreadManager()
    ->StartTimer(td, TimerThreadId());
  return 0;
}

void ClientConnect::CancelTimer() {
  NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
  if (reconnect_timer_ != 0) {
    if (net) net->CancelTimer(reconnect_timer_, TimerThreadId());
    reconnect_timer_ = 0;
  }

  if (heartbeat_timer_ != 0) {
    if (net) net->CancelTimer(heartbeat_timer_, TimerThreadId());
    heartbeat_timer_ = 0;
  }
}

void ClientConnect::OnClose() {
  //as client,if connect is closed,only closed fd,do not delete ClientConnect
  TRACE(logger_, "ClientConnect::OnClose(),fd:" << GetFd());
  if (GetStatus() == kStopping) {
    return;
  }
  if (fd_ >= 0) {
    RegisterDel(fd_);
  }
  CleanSequenceQueue();
  CancelTimer();
  CleanClient();
  StartReconnectTimer();
}

void ClientConnect::OnWrite() {
  if (GetStatus() == kConnecting) {
    OnConnectWrite();
  } else {
    Connecting::OnWrite();
  }
}

// EPOLLOUT on a pending nonblocking connect.
void ClientConnect::OnConnectWrite(){
  TRACE(logger_, "ClientConnect::OnConnectWrite,status:" << GetStatus());
  const int error = Socket::GetConnectError(fd_);
  if (error != 0) {
    INFO(logger_, "connect failed,ip:" << GetIp() << ",port:" << GetPort()
      << ",error:" << strerror(error));
    NextAddr();
    OnClose(); // arms the reconnect timer
    return;
  }
  CancelTimer(); // the connect timeout
  SetStatus(kConnected);
  // Registered read+write for the connect; keep EPOLLOUT only while data waits.
  if (0 != SetWriteInterest(false)) {
    OnClose();
    return;
  }
  StartHeartBeatTimer();
  Connecting::OnWrite();
}

bool ClientConnect::IsConnected(uint32_t time_ms, int wait_time){
  int ep_fd = epoll_create(10240);
  struct epoll_event ev = { 0 };
  ev.data.fd = GetFd();
  ev.events = EPOLLOUT;
  if (ep_fd < 0) {
    return false;
  }
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, GetFd(), &ev) == -1){
    close(ep_fd); // used to leak the epoll fd
    return false;
  }

  struct epoll_event events[10];
  while (wait_time--){
    int ret = epoll_wait(ep_fd, events, 10, time_ms);
    if (0 < ret){
      if (events[0].events & (EPOLLERR | EPOLLHUP)){
        close(ep_fd);
        return false;
      } else if (events[0].events & EPOLLOUT){
        // Writable means the connect finished; SO_ERROR says how. (Calling
        // connect() again with a sockaddr_in rebuilt from the ip broke IPv6.)
        close(ep_fd);
        return Socket::GetConnectError(GetFd()) == 0;
      }
    } else if (0 == ret){
      continue;
    } else{
      close(ep_fd);
      return false;
    }
  }
  close(ep_fd);
  return false;
}

// Runs on the owning I/O loop (heartbeat timer).
void ClientConnect::OnHeartBeat(){
  if (GetStatus() != kConnected) {
    return; // closed meanwhile: the reconnect path restarts heartbeats
  }
  EventMessage* heartbeat_message = GetProtocol()->HeartBeatRequest();
  if (nullptr != heartbeat_message) {
    // Unique ids: AsyncSequence keys on sequence_id, and the default value
    // made every heartbeat after the first a "duplicate".
    static std::atomic<uint64_t> heartbeat_seq((uint64_t)1 << 62);
    heartbeat_message->sequence_id = heartbeat_seq++;
    heartbeat_message->birthtime = Time::GetCurrentClockTime();
    if (0 != EncodeMsg(heartbeat_message)) {
      MessageFactory::Destroy(heartbeat_message);
    } else {
      Connecting::OnWrite(); // may close the connection
    }
  }
  StartHeartBeatTimer(); // no-op unless still connected
}

void ClientConnect::DoSendBack(EventMessage* message, int status) {
  if (message->type_id == MessageType::kHeartBeatMessage
    || message->direction == EventMessage::kOnlySend
    /*|| message->direction == EventMessage::kSendNoCareResponse*/) {
    MessageFactory::Destroy(message);
    return;
  }

  message->status = status;
  service::GetServiceManager().SendToServiceHandle(message);
}

void ClientConnect::CleanClient(){
  Clean();
  SetStatus(kBroken);
}

int ClientConnect::Stop(){
  EventMessage* msg = MessageFactory::Allocate<EventMessage>(0);
  msg->descriptor_id = (int64_t)((void*)this);
  service::GetServiceManager().SendCloseToNetThread(msg);
  return 0;
}

// Runs on the owning I/O loop (kIoActiveCloseEvent), like every timer of
// this connection, so nothing can fire on it after the delete.
void ClientConnect::Destroy(){
  // Unregister and cancel every timer that points at this object before
  // deleting it: the sequence timeout timers used to fire later (at the
  // latest in ~EventDriver) on freed memory and crashed shutdown.
  SetStatus(kStopping);
  if (fd_ >= 0) {
    RegisterDel(fd_);
  }
  CancelTimer();        // reconnect / heartbeat timers
  CleanSequenceQueue(); // also cancels the per-request timeout timers
  if (fd_ >= 0) {
    Clean();
  }
  BDF_DELETE(this);
}

int ClientConnect::RegisterAddModrw(int fd,int& register_thread_id){
  return service::GetServiceManager().EventAddModrw(this, fd,register_thread_id);
}

int ClientConnect::RegisterAddModr(int fd,int& register_thread_id) {
  return service::GetServiceManager().EventAddModr(this, fd,register_thread_id);
}

int ClientConnect::RegisterDel(int fd){
  if (fd <= 0) {
    return -1;
  }
  return service::GetServiceManager().EventDel(this,fd);
}

}
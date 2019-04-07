#include <sys/epoll.h>
#include "net/client_connect.h"
#include "service/io_service.h"
#include "net/socket.h"
#include "net/io_handle.h"
#include "net/client_reconnect_thread.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, ClientConnect);

ClientConnect::ClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  status_(kBroken),
  timeout_ms_(timeout_ms),
  heartbeat_ms_(heartbeat_ms),
  reconnect_timer_(0),
  heartbeat_timer_(0){
}

ClientConnect::~ClientConnect(){
}

void ClientConnect::OnTimer(void* function_data) {
  uint32_t client_timer_type = *((uint32_t*)(function_data));
  switch (client_timer_type){
  case kClientTimerTypeReconnect:
    TryConnect();
  case kClientTimerTypeHeartBeat:
    OnHeartBeat();
  default:
    WARN(logger, "unkown type:" << client_timer_type);
    break;
  }
}

int ClientConnect::TryConnect(){
  bool is_reconnect = false;
  if (reconnect_timer_ != 0){
    reconnect_timer_ = 0;
    is_reconnect = true;
  }

  SetStatus(kConnecting);
  bool is_connecting = false;
  std::pair<int, bool>connet_info = Socket::Connect(GetIp().c_str(), GetPort());
  SetFd(connet_info.first > 0 ? connet_info.first : -1);
  if (errno == EINPROGRESS){
    is_connecting = true;//正在连接
  }else if (connet_info.first < 0 || (!connet_info.second && errno != EINPROGRESS)){
    //may be not conected
    WARN(logger_, "fd:" << connet_info.first << ",status:" << connet_info.second 
      <<"connect errno:" << errno << ",EINPROGRESS" << EINPROGRESS);
    if (connet_info.first > 0){
      Socket::Close(connet_info.first);
    }
    SetStatus(kBroken);
    if (is_reconnect) StartReconnectTimer();
    return -1;
  }
  if (0 != RegisterAddModr(connet_info.first)) {
    WARN(logger_, "RegisterAddModr failed,fd:" << connet_info.first);
    Socket::Close(connet_info.first);
    SetStatus(kBroken);
    if (is_reconnect) StartReconnectTimer();
    return -2;
  }

  if (is_connecting){
    if (IsConnected(20, 3)) {
      TRACE(logger_, "has connected...");
      SetStatus(kConnected);
    } else {
      INFO(logger_, "wait 60 ms,but not conected,ip:" << GetIp() << ",port:" << GetPort());
    }
  }
  
  return 0;
}

int ClientConnect::StartReconnectTimer() {
  TRACE(logger_, "ClientConnect::StartReconnectTimer...");
  if (0 != reconnect_timer_) {
    WARN(logger, "ClientChannel::StartReconnectTimer duplicate!");
    return -1;
  }
  TimerData td;
  td.time_out_ms = 300;
  td.time_proc = this;
  td.function_data = (void*)(&client_timer_type_reconnect_);
  reconnect_timer_ = ClientReconnect::GetInstance().StartTimer(td);
  return 0;
}

int ClientConnect::StartHeartBeatTimer() {
  TRACE(logger_, "ClientConnect::StartHeartBeatTimer...");
  if (0 == heartbeat_ms_) {
    return 0;
  }

  TimerData td;
  td.time_out_ms = heartbeat_ms_;
  td.time_proc = this;
  td.function_data = (void*)(&client_timer_type_heartbeat_);

  heartbeat_timer_ = IoHandler::GetIoHandler()->StartTimer(td);
  return 0;
}

void ClientConnect::CancelTimer() {
  if (reconnect_timer_ != 0) {
    ClientReconnect::GetInstance().CancelTimer(reconnect_timer_);
    reconnect_timer_ = 0;
  }

  if (heartbeat_timer_ != 0) {
    IoHandler::GetIoHandler()->CancelTimer(heartbeat_timer_);
    heartbeat_timer_ = 0;
  }
}

void ClientConnect::OnClose() {
  //as client,if connect is closed,only closed fd,do not delete ClientConnect
  TRACE(logger_, "ClientConnect::OnClose(),fd:" << GetFd());
  CleanSequenceQueue();
  CancelTimer();
  CleanClient();
  RegisterDel(GetFd());
  StartReconnectTimer();
}

void ClientConnect::OnWrite() {
  if (GetStatus() == kConnecting) {
    OnConnectWrite();
  } else {
    Connecting::OnWrite();
  }
}

void ClientConnect::OnConnectWrite(){
  TRACE(logger_, "ClientConnect::OnConnectWrite,status:" << GetStatus());
  if (IsConnected(10, 3)) {
    SetStatus(kConnected);
  } else {
    TRACE(logger_, "OnConnectWrite wait 60 ms,but not conected.");
    //对于一般服务和服务器90ms，连接时间够用，如果是网络很差可能出现一直重连，又连不上的情况
    CleanClient();
    StartHeartBeatTimer();
    return;
  }

  if (0 != RegisterAddModr(GetFd())) {
    WARN(logger_, "RegisterAddModr failed,fd:" << GetFd());
    CleanClient();
    StartHeartBeatTimer();
    return ;
  }

  StartHeartBeatTimer();
  Connecting::OnWrite();
}

bool ClientConnect::IsConnected(uint32_t time_ms, int wait_time){
  int ep_fd = epoll_create(10240);
  struct epoll_event ev = { 0 };
  ev.data.fd = GetFd();
  ev.events = EPOLLOUT;
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, GetFd(), &ev) == -1){
    return false;
  }

  sockaddr_in addr = Socket::GenerateAddr(GetIp().c_str(), GetPort());
  struct epoll_event events[10];
  while (wait_time--){
    int ret = epoll_wait(ep_fd, events, 10, time_ms);
    if (0 < ret){
      if (events[0].events & (EPOLLERR | EPOLLHUP)){
        close(ep_fd);
        return false;
      } else if (events[0].events & EPOLLOUT){
        int res = connect(GetFd(), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
        if (0 == res || errno == EISCONN){
          close(ep_fd);
          return true;
        } else if (errno == EALREADY || errno == EINPROGRESS){
          continue;
        } else{
          close(ep_fd);
          return false;
        }
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

void ClientConnect::OnHeartBeat(){
  EventMessage* heartbeat_message = GetProtocol()->HeartBeatRequest();
  if (0 != EncodeMsg(heartbeat_message)) {
    MessageFactory::Destroy(heartbeat_message);
  }

  OnWrite();
  StartHeartBeatTimer();
}

void ClientConnect::DoSendBack(EventMessage* message, int status) {
  if (message->type_id == MessageType::kHeartBeatMessage
    || message->direction == EventMessage::kOnlySend
    || message->direction == EventMessage::kSendNoCareResponse) {
    MessageFactory::Destroy(message);
    return;
  }

  message->status = status;
  IoService::GetInstance().SendToServiceHandle(message);
}

void ClientConnect::CleanClient(){
  Clean();
  SetStatus(kBroken);
}

int ClientConnect::Stop(){
  EventMessage* msg = MessageFactory::Allocate<EventMessage>(0);
  IoService::GetInstance().SendCloseToIoHandle(msg);
  return 0;
}

int ClientConnect::RegisterAddModrw(int fd){
  return IoService::GetInstance().AgentsAddModrw(this, fd);
}

int ClientConnect::RegisterAddModr(int fd) {
  return IoService::GetInstance().AgentsAddModr(this, fd);
}

int ClientConnect::RegisterDel(int fd){
  return IoService::GetInstance().AgentsDel(this,fd);
}

}
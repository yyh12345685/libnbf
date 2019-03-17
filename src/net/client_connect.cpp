#include "net/client_connect.h"
#include "service/io_service.h"
#include "net/socket.h"
#include "net/io_handle.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, ClientConnect);

ClientConnect::ClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  timeout_ms_(timeout_ms),
  heartbeat_ms_(heartbeat_ms){
}

ClientConnect::~ClientConnect(){
}

void ClientConnect::OnTimer(void* function_data) {
}

int ClientConnect::TryConnect(){

  SetStatus(kConnecting);

  std::pair<int, bool>connet_info = Socket::Connect(GetIp().c_str(), GetPort());
  int error_no = errno;
  if (connet_info.first < 0 || (!connet_info.second && errno != EINPROGRESS)){
    //may be not conected
    WARN(logger_, "fd:" << connet_info.first << ",status:" << connet_info.second 
      <<"connect errno:" << errno << ",EINPROGRESS" << EINPROGRESS);
    if (connet_info.first > 0){
      Socket::Close(connet_info.first);
    }
    SetStatus(kBroken);
    return -1;
  }
  if (0 != RegisterAddModrw(connet_info.first)) {
    WARN(logger_, "RegisterAddModrw failed,fd:" << connet_info.first);
    Socket::Close(connet_info.first);
    SetStatus(kBroken);
    return -2;
  }
  
  return 0;
}

int ClientConnect::StartReconnectTimer() {
  if (0 != reconnect_timer_) {
    WARN(logger, "ClientChannel::StartReconnectTimer duplicate!");
    return -1;
  }
  TimerData td;
  td.time_out_ms = 1000;
  td.time_proc = this;
  td.function_data = &client_timer_type_reconnect_;
  reconnect_timer_ = IoHandler::GetIoHandler()->StartTimer(td);
  return 0;
}

int ClientConnect::StartTimeoutTimer() {
  if (0 == timeout_ms_) {
    return 0;
  }

  TimerData td;
  td.time_out_ms = 5;
  td.time_proc = this;
  td.function_data = &client_timer_type_timeout_;

  timeout_timer_ = IoHandler::GetIoHandler()->StartTimer(td);
  return 0;
}

int ClientConnect::StartHeartBeatTimer() {
  if (0 == heartbeat_ms_) {
    return 0;
  }

  TimerData td;
  td.time_out_ms = heartbeat_ms_;
  td.time_proc = this;
  td.function_data = &client_timer_type_heartbeat_;

  heartbeat_timer_ = IoHandler::GetIoHandler()->StartTimer(td);
  return 0;
}

void ClientConnect::CancelTimer() {
  if (reconnect_timer_ != 0) {
    IoHandler::GetIoHandler()->CancelTimer(reconnect_timer_);
    reconnect_timer_ = 0;
  }

  if (timeout_timer_ != 0) {
    IoHandler::GetIoHandler()->CancelTimer(timeout_timer_);
    timeout_timer_ = 0;
  }

  if (heartbeat_timer_ != 0) {
    IoHandler::GetIoHandler()->CancelTimer(heartbeat_timer_);
    heartbeat_timer_ = 0;
  }
}

void ClientConnect::OnClose() {
  //as client,if connect is closed,only closed fd,do not delete ClientConnect
  //PurgeSequenceKeeper();
  CancelTimer();
  Clean();

  SetStatus(kBroken);
  StartReconnectTimer();
}

int ClientConnect::Stop(){
  EventMessage* msg = MessageFactory::Allocate<EventMessage>(0);
  IoService::GetInstance().SendCloseToIoHandle(msg);
  return 0;
}

int ClientConnect::RegisterAddModrw(int fd) {
  IoService::GetInstance().AgentsAddModr(this, fd);
  return 0;
}

int ClientConnect::RegisterDel(int fd){
  IoService::GetInstance().AgentsDel(this,fd);
  return 0;
}

}

#include <atomic>
#include "net/server_connect.h"
#include "common/time.h"
#include "service/service_manager.h"
#include "net/connect_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_threads.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ServerConnect);

ServerConnect::ServerConnect(){
  static std::atomic<int64_t> server_connect_id(0);
  SetConnectId(server_connect_id.fetch_add(1, std::memory_order_relaxed));
}

ServerConnect::~ServerConnect(){
  // Shutdown deletes still-open connections (delay release manager); a
  // pending linger timer would otherwise fire on freed memory.
  CancelLinger();
}

void ServerConnect::OnClose() {
  if (fd_ < 0) {
    return;
  }
  //as server,if connect is closed,can delete Connecting
  //as client,if connect is closed,only closed fd
  CancelLinger();
  RegisterDel(fd_);
  Clean();
  //if (ConnectManager::Instance().GetConnect((uint64_t)this)){
  //  ConnectManager::Instance().UnRegisterConnect((uint64_t)this);
    Destroy();
  //}
}

void ServerConnect::Destroy(){
  // Also reached from kIoActiveCloseEvent with the fd still open: unregister
  // and close it first, or the fd stays registered with a pointer that
  // dangles once the delayed delete runs.
  CancelLinger();
  if (fd_ >= 0) {
    RegisterDel(fd_);
    Clean();
  }
  if (released_) {
    return; // OnClose and an active close both end here
  }
  released_ = true;
  service::GetServiceManager().ReleaseServerCon(this);
  //BDF_DELETE(this);
}

void ServerConnect::OnDecodeMessage(EventMessage* message) {
  if (message->type_id == MessageType::kHeartBeatMessage) {
    EventMessage* heartbeat_response = GetProtocol()->HeartBeatResponse(message);
    // Encode copies into outbuf_, so the message is freed on success too
    // (it used to leak one message per heartbeat / favicon request).
    int ret = EncodeMsg(heartbeat_response);
    MessageFactory::Destroy(heartbeat_response);
    if (0 == ret) {
      OnWrite(); // decode runs on this connection's I/O thread
    }
    return;
  }

  message->birthtime = Time::GetCurrentClockTime();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingRequest;
  message->descriptor_id = MakeDescriptor(this, GetConnectId());

  ++in_flight_;
  service::GetServiceManager().SendToServiceHandle(message);
}

void ServerConnect::OnPeerClosed(){
  if (peer_closed_ || fd_ < 0) {
    return;
  }
  peer_closed_ = true;
  if (in_flight_ == 0 && outbuf_.GetReadingSize() == 0) {
    graceful_close_ = true;
    OnClose();
    return;
  }
  // Bound the wait: a handler may never answer (e.g. overload protection).
  TimerData td;
  td.time_out_ms = kHalfCloseLingerMs;
  td.time_proc = this;
  td.function_data = nullptr;
  NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
  linger_timer_ = net->StartTimer(td, net->GetIoThreads()->GetIoThreadId(this));
}

void ServerConnect::OnTimer(void* function_data){
  linger_timer_ = 0;
  if (fd_ >= 0) {
    INFO(logger_, "half-closed connection timed out waiting for replies,fd:" << fd_);
    OnClose();
  }
}

void ServerConnect::CancelLinger(){
  if (linger_timer_ != 0) {
    NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
    if (net) net->CancelTimer(linger_timer_, net->GetIoThreads()->GetIoThreadId(this));
    linger_timer_ = 0;
  }
}

void ServerConnect::OnWrite(){
  Connecting::OnWrite();
  MaybeFinishHalfClose();
}

void ServerConnect::MaybeFinishHalfClose(){
  if (peer_closed_ && fd_ >= 0 && in_flight_ == 0 && outbuf_.GetReadingSize() == 0) {
    graceful_close_ = true; // let the kernel deliver what is queued
    OnClose();
  }
}

int ServerConnect::RegisterDel(int fd){
  if (fd <= 0){
    return -1;
  }
  return service::GetServiceManager().EventDel(this, fd);
}

}

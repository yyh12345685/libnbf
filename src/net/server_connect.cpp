
#include <atomic>
#include "net/server_connect.h"
#include "common/time.h"
#include "service/service_manager.h"
#include "net/connect_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ServerConnect);

ServerConnect::ServerConnect(){
  static std::atomic<int64_t> server_connect_id(0);
  SetConnectId(server_connect_id.fetch_add(1, std::memory_order_relaxed));
}

ServerConnect::~ServerConnect(){
}

void ServerConnect::OnClose() {
  if (fd_ < 0) {
    return;
  }
  //as server,if connect is closed,can delete Connecting
  //as client,if connect is closed,only closed fd
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
  message->descriptor_id = (int64_t)((void*)this);

  service::GetServiceManager().SendToServiceHandle(message);
}

int ServerConnect::RegisterDel(int fd){
  if (fd <= 0){
    return -1;
  }
  return service::GetServiceManager().EventDel(this, fd);
}

}

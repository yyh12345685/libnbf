
#include "net/server_connect.h"
#include "common/time.h"
#include "service/io_service.h"
#include "net/connect_manager.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, ServerConnect);

ServerConnect::ServerConnect(){
}

ServerConnect::~ServerConnect(){
}

void ServerConnect::OnClose() {
  //as server,if connect is closed,can delete Connecting
  //as client,if connect is closed,only closed fd
  RegisterDel(fd_);
  Clean();
  if (ConnectManager::Instance().GetConnect((uint64_t)this)){
    ConnectManager::Instance().UnRegisterConnect((uint64_t)this);
    //·ÀÖ¹ÖØ¸´Îö¹¹
    Destroy();
  }
}

void ServerConnect::OnDecodeMessage(EventMessage* message) {
  if (message->type_id == MessageType::kHeartBeatMessage) {
    EventMessage* heartbeat_response = GetProtocol()->HeartBeatResponse(message);
    if (0 != EncodeMsg(heartbeat_response)) {
      MessageFactory::Destroy(heartbeat_response);
    }
    return;
  }

  message->birthtime = Time::GetMillisecond();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingRequest;
  message->descriptor_id = (int64_t)((void*)this);

  IoService::GetInstance().SendToServiceHandle(message);
}

int ServerConnect::RegisterDel(int fd){
  if (fd <= 0){
    return -1;
  }
  return IoService::GetInstance().AgentsDel(this, fd);
}

}
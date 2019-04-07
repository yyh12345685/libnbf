
#include "net/server_connect.h"
#include "common/time.h"
#include "service/io_service.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, ServerConnect);

ServerConnect::ServerConnect(){
}

ServerConnect::~ServerConnect(){
}

void ServerConnect::OnClose() {
  //as server,if connect is closed,can delete Connecting
  //as client,if connect is closed,only closed fd
  RegisterDel(GetFd());
  Clean();
  Destroy();
}

void ServerConnect::OnDecodeMessage(EventMessage* message) {

  message->birthtime = Time::GetMillisecond();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingRequest;
  message->descriptor_id = (int64_t)((void*)this);

  IoService::GetInstance().SendToServiceHandle(message);
}

int ServerConnect::RegisterDel(int fd){
  return IoService::GetInstance().AgentsDel(this, fd);
}

}
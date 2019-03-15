#include "net/client_connect.h"
#include "service/io_service.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, ClientConnect);

void ClientConnect::OnTimer(void* function_data) {
}

int ClientConnect::TryConnect(){
  return 0;
}

int ClientConnect::Stop(){
  EventMessage* msg = MessageFactory::Allocate<EventMessage>(0);
  IoService::GetInstance().SendCloseToIoHandle(msg);
  return 0;
}

int ClientConnect::RegisterAddModrw(int fd, bool set) {
  return 0;
}

int ClientConnect::RegisterDel(int fd){
  return 0;
}

}
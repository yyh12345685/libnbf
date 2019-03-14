#include "net/client_connect.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, ClientConnect);

void ClientConnect::OnTimer(void* function_data) {
}

void ClientConnect::TryConnect(){

}

int ClientConnect::RegisterAddModr(int fd, bool set){
  return 0;
}

int ClientConnect::RegisterModr(int fd, bool set){
  return 0;
}

int ClientConnect::RegisterModw(int fd, bool set){
  return 0;
}

int ClientConnect::RegisterDel(int fd){
  return 0;
}

}
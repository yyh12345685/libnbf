#include "net/connect.h"
#include "event/event_driver.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, Connecting);

void Connecting::OnEvent(EventDriver *poll, int fd, short event){
  if (event & EVENT_CONNECT_CLOSED){
    OnCloseConnection(fd);
    return;
  }

  if (event & EVENT_READ){
    OnRead();
  }
  if (event & EVENT_WRITE){
    OnWrite();
  }
  if (event & EVENT_ERROR){
    TRACE(logger_, "Connecting::OnEvent,EVENT_ERROR.");
    OnError();
  }
}

void Connecting::OnCloseConnection(int fd){

}
void Connecting::OnError(){

}

}

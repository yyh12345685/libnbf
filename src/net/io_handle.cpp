
#include <unistd.h>
#include "net/io_handle.h"
#include "message_base.h"
#include "handle_data.h"
#include "net/connect.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, IoHandler);

void IoHandler::Run(void* handle, HandleData* data){
  IoHandler* io_handle = (IoHandler*)handle;
  while (data->is_run) {
    if (data->data_.empty()) {
      usleep(10);
      continue;
    }

    std::queue<EventMessage*> temp;
    data->lock_.lock();
    temp.swap(data->data_);
    data->lock_.unlock();

    while (!temp.empty()) {
      EventMessage *msg = temp.front();
      temp.pop();
      io_handle->Handle(msg);
    }
  }
}

void IoHandler::Handle(EventMessage* message){
  TRACE(logger, "IoHandler::Handle " << *message);

  switch (message->type_io_event) {
    case MessageType::kIoMessageEvent:
    HandleIoMessageEvent(message);
    break;
  case MessageType::kIoActiveCloseEvent:
    HandleIoActiveCloseEvent(message);
    break;
  default:
    WARN(logger, "IOHanlder unknown message:" << message->type_id);
    break;
  }

  MessageFactory::Destroy(message);
}

void IoHandler::HandleIoMessageEvent(EventMessage* message){
  TRACE(logger, "HandleIOMessageEvent");
  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger, "IoHandler::HandleIoMessageEventt descriptor is not Connecting");
    return;
  }

  if (0 != con->EncodeMsg(message)) {
    return;
  }

  con->OnWrite();
}

void IoHandler::HandleIoActiveCloseEvent(EventMessage* message){
  TRACE(logger, "HandleIOMessageEvent");
  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger, "IoHandler::HandleIoMessageEventt descriptor is not Connecting");
    return;
  }

  con->OnActiveClose();
}

}


#include <unistd.h>
#include "service/service_handle.h"
#include "message.h"
#include "handle_data.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, ServiceHandler);

void ServiceHandler::Run( HandleData* data){
  while (data->is_run) {
    if (data->data_.empty()){
      usleep(1);
      continue;
    }

    std::queue<EventMessage*> temp;
    data->lock_.lock();
    temp.swap(data->data_);
    data->lock_.unlock();

    while (!temp.empty()){
      EventMessage *msg = temp.front();
      temp.pop();
      Handle(msg);
    }
  }
}

void ServiceHandler::Handle(EventMessage* message) {
  //if (message->status != EventMessage::kStatusOK) {
  //  MessageFactory::Destroy(message);
  //  return;
  //}
  switch (message->type_id) {
  case MessageType::kHttpMessage:
    return message->direction == MessageBase::kIncomingRequest
      ? OnHttpRequestMessage(static_cast<HttpMessage*>(message))
      : OnHttpResponseMessage(static_cast<HttpMessage*>(message));
  case MessageType::kRapidMessage:
    return message->direction == MessageBase::kIncomingRequest
      ? OnRapidRequestMessage(static_cast<RapidMessage*>(message))
      : OnRapidResponseMessage(static_cast<RapidMessage*>(message));
  default:
    ERROR(logger,"unkown message,type_id:"<< message->type_id);
  }
}

void ServiceHandler::OnHttpRequestMessage(HttpMessage* message){
  WARN(logger, "not should here,not implement:" << message->type_id);
}

//if use coroutine,should not handle
void ServiceHandler::OnHttpResponseMessage(HttpMessage* message){
  return;
  //WARN(logger, "not should here,not implement:" << message->type_id);
}

void ServiceHandler::OnRapidRequestMessage(RapidMessage* message){
  WARN(logger, "not should here,not implement:" << message->type_id);
}

//if use coroutine,should not handle
void ServiceHandler::OnRapidResponseMessage(RapidMessage* message){
  return;
  //WARN(logger, "not should here,not implement:" << message->type_id);
}

}


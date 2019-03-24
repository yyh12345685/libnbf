#include "app_test_server_handle.h"
#include "message.h"
#include "service/io_service.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, AppTestServerHandler);

//server receive request
void AppTestServerHandler::OnRapidRequestMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);

  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid protocol, response hello world...";
  service::GetIoService().Reply(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void AppTestServerHandler::OnRapidResponseMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidResponseMessage:" << *message);

}

}

}

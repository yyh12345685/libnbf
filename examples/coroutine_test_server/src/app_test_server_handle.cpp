#include "app_test_server_handle.h"
#include "message.h"
#include "service/service_manager.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, AppTestServerHandler);

//server receive request
void AppTestServerHandler::OnRapidRequestMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);
  if (0xffffffffffffffff == message->sequence_id){
    //rapid protocol no need response
    MessageFactory::Destroy(message);
    return;
  }
  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid protocol, response hello world...";
  service::GetServiceManager().Reply(msg);
  MessageFactory::Destroy(message);
}

//server receive request
void AppTestServerHandler::OnHttpRequestMessage(HttpMessage* message) {
  TRACE(logger_, "OnHttpRequestMessage:" << *message);
  HttpMessage* msg = MessageFactory::Allocate<HttpMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http protocol,response hello world---------";
  service::GetServiceManager().Reply(msg);
  MessageFactory::Destroy(message);
}

}

}

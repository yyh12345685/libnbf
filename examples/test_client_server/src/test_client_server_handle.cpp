#include "test_client_server_handle.h"
#include "message.h"
#include "service/io_service.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, TestClientServerHandler);

//server receive request
void TestClientServerHandler::OnHttpRequestMessage(HttpMessage* message){
  TRACE(logger_, "OnHttpRequestMessage:"<< *message);
  HttpMessage* msg = MessageFactory::Allocate<HttpMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http protocol,response hello world---------";
  service::GetIoService().SendToIoHandle(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnHttpResponseMessage(HttpMessage* message){
  TRACE(logger_, "OnHttpResponseMessage:" << *message);

}

//server receive request
void TestClientServerHandler::OnRapidRequestMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);

  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid protocol, response hello world...";
  service::GetIoService().SendToIoHandle(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnRapidResponseMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidResponseMessage:" << *message);

}

}

}

#include "test_server_handle.h"
#include "message.h"
#include "service/io_service.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, TestHandler);

void TestHandler::OnHttpRequestMessage(HttpMessage* message){
  INFO(logger_, "OnRapidRequestMessage");
  HttpMessage* msg = MessageFactory::Allocate<HttpMessage>();
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http protocol,hello world---------";
  service::GetIoService().SendToIoHandle(msg);
}

void TestHandler::OnRapidRequestMessage(RapidMessage* message){
  INFO(logger_, "OnRapidRequestMessage");
  message->body = "response";

  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid protocol, hello world...";
  service::GetIoService().SendToIoHandle(msg);
}

}

}

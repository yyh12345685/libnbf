#include "test_client_server_handle.h"
#include "message.h"
#include "service/io_service.h"
#include "monitor/matrix_scope.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, TestClientServerHandler);

//server receive request
void TestClientServerHandler::OnHttpRequestMessage(HttpMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnHttpRequestMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnHttpRequestMessage info:"<< *message);
  HttpMessage* msg = BDF_NEW(HttpMessage);
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http response hello world.";
  service::GetIoService().Reply(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnHttpResponseMessage(HttpMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnHttpResponseMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnHttpResponseMessage:" << *message);
  if (EventMessage::kStatusOK !=message->status){
    matrix_scope.SetOkay(false);
  }
  MessageFactory::Destroy(message);
}

//server receive request
void TestClientServerHandler::OnRapidRequestMessage(RapidMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnRapidRequestMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);
  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid response hello world.";
  service::GetIoService().Reply(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnRapidResponseMessage(RapidMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnRapidResponseMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnRapidResponseMessage:" << *message);
  if (EventMessage::kStatusOK != message->status) {
    matrix_scope.SetOkay(false);
  }
  MessageFactory::Destroy(message);
}

}

}

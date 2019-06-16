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
  TRACE(logger_, "OnHttpRequestMessage:"<< *message);
  //测试客户端不带接收功能，所以注释掉发送
  HttpMessage* msg = MessageFactory::Allocate<HttpMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http protocol,response hello world---------";
  service::GetIoService().Reply(msg);
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnHttpResponseMessage(HttpMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnHttpResponseMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnHttpResponseMessage:" << *message);

  MessageFactory::Destroy(message);
}

//server receive request
void TestClientServerHandler::OnRapidRequestMessage(RapidMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnRapidRequestMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);
  //测试客户端不带接收功能，所以注释掉发送
  /*RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid protocol, response hello world...";
  service::GetIoService().Reply(msg);*/
  MessageFactory::Destroy(message);
}

//client receive request
void TestClientServerHandler::OnRapidResponseMessage(RapidMessage* message){
  bdf::monitor::MatrixScope matrix_scope(
    "ServerTestOnRapidResponseMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  TRACE(logger_, "OnRapidResponseMessage:" << *message);

  MessageFactory::Destroy(message);
}

}

}

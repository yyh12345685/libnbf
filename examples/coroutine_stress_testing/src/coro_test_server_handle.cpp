#include <unistd.h>
#include "coro_test_server_handle.h"
#include "message.h"
#include "service/service_manager.h"
#include "monitor/matrix_scope.h"
#include "client_task_test.h"

namespace bdf {

namespace testserver {

LOGGER_CLASS_IMPL(logger_, CoroTestServerHandler);

//server receive request
void CoroTestServerHandler::OnRapidRequestMessage(RapidMessage* message){
  TRACE(logger_, "OnRapidRequestMessage,body:" << message->body);
  bdf::monitor::MatrixScope matrix_scope(
    "OnRapidRequestMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  if (0xffffffffffffffff == message->sequence_id){
    //rapid protocol no need response
    MessageFactory::Destroy(message);
    return;
  }
  //usleep(1000 * 100);

#ifdef CLIENT_TEST
  ClientTaskTest::HttpClientTestSigle();
#endif

  RapidMessage* msg = MessageFactory::Allocate<RapidMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->sequence_id = message->sequence_id;
  msg->body = "rapid response hello .";
  service::GetServiceManager().Reply(msg);
  MessageFactory::Destroy(message);
}

//server receive request
void CoroTestServerHandler::OnHttpRequestMessage(HttpMessage* message) {
  TRACE(logger_, "OnHttpRequestMessage:" << *message);
  bdf::monitor::MatrixScope matrix_scope(
    "OnHttpRequestMessage", bdf::monitor::MatrixScope::kModeAutoSuccess);
  //usleep(1000 * 200);

#ifdef CLIENT_TEST
  ClientTaskTest::RapidClientTestSigle();
#endif

  HttpMessage* msg = MessageFactory::Allocate<HttpMessage>();
  msg->SetDescriptorId(message->GetDescriptorId());
  msg->InitReply(message, 200, false);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "http response world.";
  service::GetServiceManager().Reply(msg);
  MessageFactory::Destroy(message);
}

}

}

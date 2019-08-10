#include "app/app.h"
#include "client/client_mgr.h"
#include "message.h"
#include "coro_test_server_handle.h"
#include "task.h"
#include "service/io_service.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

void RapidClientTestSigle(){
  TRACE(logger_, "RapidTest SendRecieve.");
  bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message_2->body = "test send recieve rapid";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2);
  if (nullptr == msg2_resp) {
    INFO(logger_, "msg2_resp is null.");
    return;
  }
  bdf::RapidMessage* real_msg = dynamic_cast<bdf::RapidMessage*>(msg2_resp);
  TRACE(logger_, "receive msg:" << *real_msg);
  bdf::MessageFactory::Destroy(real_msg);
}

void HttpClientTestSigle(){
  TRACE(logger_, "HttpTest SendRecieve.");
  bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  hmsg2->InitRequest("POST", true);
  hmsg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg2->http_info.url = "/mm?a=xx&b=yy";
  hmsg2->http_info.body = "HttpTest send receive:hello world.";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("http_test_client", hmsg2);
  if (nullptr == msg2_resp) {
    INFO(logger_, "msg2_resp is nullptr.");
    return;
  }
  bdf::HttpMessage* hmsg2_resp = dynamic_cast<bdf::HttpMessage*>(msg2_resp);
  TRACE(logger_, "receive msg:" << *hmsg2_resp);
  bdf::MessageFactory::Destroy(hmsg2_resp);
}

class ClientTaskTest :public bdf::Task {
  virtual void OnTask(void* function_data) {
    INFO(logger_, "start task.");
    while (true) {
      RapidClientTestSigle();
      HttpClientTestSigle();
    }
    INFO(logger_, "exit task.");
  }
};

int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::CoroTestServerHandler> app;
  ClientTaskTest client_test_task;
  app.SetOnAfterStart([&]() {
    uint32_t service_handle_cnt = bdf::IoService::GetInstance().GetServiceHandleCount();
    for (uint32_t idx = 0; idx < service_handle_cnt;idx++) {
      bdf::service::GetIoService().SendTaskToServiceHandle(&client_test_task);
    }
    return 0;
  });
  return app.Run(argc, argv);
}


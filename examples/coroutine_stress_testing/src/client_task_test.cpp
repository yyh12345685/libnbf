#include "client_task_test.h"
#include "client/client_mgr.h"
#include "message.h"
#include "app/app.h"

LOGGER_CLASS_IMPL(logger_, ClientTaskTest);

void ClientTaskTest::RapidClientTestSigleSendOnly() {
  TRACE(logger_, "RapidTest SendOnly.");
  bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message_2->body = "test send only rapid";
  bdf::AppBase::Get()->GetClientMgr()->Send("rapid_test_client", rapid_message_2);
}

void ClientTaskTest::RapidClientTestSigle() {
  TRACE(logger_, "RapidTest SendRecieve.");
  bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message_2->body = "send recieve rapid.";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2);
  if (nullptr == msg2_resp) {
    TRACE(logger_, "msg2_resp is null.");
    return;
  }
  bdf::RapidMessage* real_msg = dynamic_cast<bdf::RapidMessage*>(msg2_resp);
  if (nullptr == real_msg){
    WARN(logger_, "real_msg is null.");
    return;
  }
  TRACE(logger_, "receive msg:" << *real_msg);
  bdf::MessageFactory::Destroy(real_msg);
}

void ClientTaskTest::HttpClientTestSigleSendOnly() {
  TRACE(logger_, "HttpTest SendOnly.");
  bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  hmsg2->InitRequest("POST", true);
  hmsg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg2->http_info.url = "/oy?a=xx&b=yy";
  hmsg2->http_info.body = "HttpTest send only:send only.";
  bdf::AppBase::Get()->GetClientMgr()->Send("http_test_client", hmsg2);
}

void ClientTaskTest::HttpClientTestSigle() {
  TRACE(logger_, "HttpTest SendRecieve.");
  bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  hmsg2->InitRequest("POST", true);
  hmsg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg2->http_info.url = "/mm?a=xx";
  hmsg2->http_info.body = "HttpTest send receive.";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("http_test_client", hmsg2);
  if (nullptr == msg2_resp) {
    TRACE(logger_, "msg2_resp is nullptr.");
    return;
  }
  bdf::HttpMessage* hmsg2_resp = dynamic_cast<bdf::HttpMessage*>(msg2_resp);
  if (nullptr == hmsg2_resp) {
    WARN(logger_, "real_msg is null.");
    return;
  }
  TRACE(logger_, "receive msg:" << *hmsg2_resp);
  bdf::MessageFactory::Destroy(hmsg2_resp);
}

void ClientTaskTest::OnTask(void* function_data) {
  TRACE(logger_, "start task.");
  while (true) {
    RapidClientTestSigle();
    //RapidClientTestSigle();
    HttpClientTestSigle();
    //HttpClientTestSigle();
    break;
  }
  TRACE(logger_, "exit task.");
}
#include <unistd.h>
#include <sys/prctl.h>
#include "app/app.h"
#include "app/appbase.h"
#include "client/client_mgr.h"
#include "message.h"
#include "coro_test_server_handle.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

void RapidClientTestSigle(bdf::Application<bdf::testserver::CoroTestServerHandler>* app){
  TRACE(logger_, "RapidTest SendRecieve.");
  bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message_2->body = "test send recieve rapid";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2);
  if (nullptr == msg2_resp) {
    WARN(logger_, "msg2_resp is null.");
    return;
  }
  bdf::RapidMessage* real_msg = dynamic_cast<bdf::RapidMessage*>(msg2_resp);
  TRACE(logger_, "receive msg:" << *real_msg);
  bdf::MessageFactory::Destroy(real_msg);
}

int RapidClientTest(void* data) {
  prctl(PR_SET_NAME, "RapidClientTest111111");
  bdf::Application<bdf::testserver::CoroTestServerHandler>* app =
    (bdf::Application<bdf::testserver::CoroTestServerHandler>*)data;

  while (true){
    RapidClientTestSigle(app);
  }
  return 0;
}

void HttpClientTestSigle(bdf::Application<bdf::testserver::CoroTestServerHandler>* app){
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
    WARN(logger_, "msg2_resp is nullptr.");
    return;
  }
  bdf::HttpMessage* hmsg2_resp = dynamic_cast<bdf::HttpMessage*>(msg2_resp);
  TRACE(logger_, "receive msg:" << *hmsg2_resp);
  bdf::MessageFactory::Destroy(hmsg2_resp);
}

int HttpClientTest(void* data) {
  prctl(PR_SET_NAME, "RapidClientTest222222");
  bdf::Application<bdf::testserver::CoroTestServerHandler>* app =
    (bdf::Application<bdf::testserver::CoroTestServerHandler>*)data;

  while (true){
    HttpClientTestSigle(app);
  }
}

void StartRapidTest(void* app){
  int thread_num = 2;
  std::vector<std::thread*> threads_rapid;
  for (int idx = 0; idx < thread_num; idx++) {
    std::thread* rt = new std::thread(&RapidClientTest, &app);
    threads_rapid.push_back(rt);
  }

  for (int idx = 0; idx < thread_num; idx++){
    threads_rapid[idx]->join();
    delete threads_rapid[idx];
  }
}

void StartHttpTest(void* app){
  int thread_num = 2;
  std::vector<std::thread*> threads_http;
  for (int idx = 0; idx < thread_num; idx++) {
    std::thread* rt = new std::thread(&RapidClientTest, &app);
    threads_http.push_back(rt);
  }

  for (int idx = 0; idx < thread_num; idx++){
    threads_http[idx]->join();
    delete threads_http[idx];
  }
}

int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::CoroTestServerHandler> app;
  app.SetOnAfterStart([&]() {
    StartRapidTest(&app);
    StartHttpTest(&app);
    return 0;
  });
  return app.Run(argc, argv);
}

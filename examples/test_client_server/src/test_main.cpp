#include "app/app.h"
#include "test_client_server_handle.h"
#include "message.h"
#include "message_base.h"
#include "client/client_mgr.h"
#include "monitor/matrix_scope.h"

LOGGER_STATIC_DECL_IMPL(logger, "root");

int RapidClientTest(void* data) {

  TRACE(logger, "RapidClientTest start");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app = 
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;
  bdf::ForTest::Inst().SetForTest(true);
  int times = 1000000;
  while (times-- > 0){
    bdf::monitor::MatrixScope matrix_scope(
      "RapidClient", bdf::monitor::MatrixScope::kModeAutoSuccess);
    bdf::RapidMessage* rapid_message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
    rapid_message->body = "test rapid_test_client:hello world.";
    app->GetClientMgr()->Send("rapid_test_client", rapid_message);

    bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
    rapid_message_2->body = "aaa";
    bdf::EventMessage* msg2_resp =
      app->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2, 10);
    if (nullptr == msg2_resp){
      TRACE(logger, "msg2_resp is null.");
      //continue;
      break;
    }
    TRACE(logger, "receive msg:" << *msg2_resp);
    if (msg2_resp->status != bdf::EventMessage::kStatusOK) {
      WARN(logger, "time out message");
    }
    bdf::MessageFactory::Destroy(msg2_resp);
    //break;
  }
  
  return 0;
}

int HttpClientTest(void* data) {
  TRACE(logger, "HttpClientTest start");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app =
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;
  bdf::ForTest::Inst().SetForTest(true);
  int times = 10000000;
  while (times-- > 0) {
    bdf::monitor::MatrixScope matrix_scope(
      "HttpClient", bdf::monitor::MatrixScope::kModeAutoSuccess);
    bdf::HttpMessage* hmsg = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
    hmsg->InitRequest("POST", true);
    hmsg->http_info.headers.insert(
      std::pair<std::string, std::string>("Content-Type", "text/html"));
    hmsg->http_info.url = "/a=x&b=y";
    hmsg->http_info.body = "test http_test_client send:hello world.";
    app->GetClientMgr()->Send("http_test_client", hmsg);

    bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
    hmsg2->InitRequest("POST", true);
    hmsg2->http_info.headers.insert(
      std::pair<std::string, std::string>("Content-Type", "text/html"));
    hmsg2->http_info.url = "/a=xx&b=yy";
    hmsg2->http_info.body = "test http_test_client send receive:hello world.";
    bdf::EventMessage* hmsg2_resp =
      app->GetClientMgr()->SendRecieve("http_test_client", hmsg2, 10);
    if (nullptr == hmsg2_resp) {
      TRACE(logger, "msg2_resp is null.");
      //continue;
      break;
    }
    TRACE(logger, "receive msg:" << *hmsg2_resp);

    if (hmsg2_resp->status != bdf::EventMessage::kStatusOK){
      WARN(logger, "time out message");
    }
    bdf::MessageFactory::Destroy(hmsg2_resp);
    //break;
  }
  return 0;
}

int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::TestClientServerHandler> app;
  app.SetOnAfterStart([&](){
    std::thread t1(&RapidClientTest, &app);
    std::thread t2(&HttpClientTest, &app);
    t1.join();
    t2.join();
    return 0;
  });
  return app.Run(argc,argv);
}

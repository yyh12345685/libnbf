#include "app/app.h"
#include "test_client_server_handle.h"
#include "message.h"
#include "message_base.h"
#include "client/client_mgr.h"

LOGGER_STATIC_DECL_IMPL(logger, "root");

int RapidClientTest(void* data) {

  TRACE(logger, "RapidClientTest start");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app = 
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;

  bdf::RapidMessage* rapid_message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message->body = "test rapid_test_client:hello world.";
  app->GetClientMgr()->Send("rapid_test_client", rapid_message);

  bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  rapid_message_2->body = "aaa";
  bdf::EventMessage* msg2_resp =
    app->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2, 10);
  if (nullptr != msg2_resp) {
    TRACE(logger, "receive msg:" << *msg2_resp);
  }
  return 0;
}

int HttpClientTest(void* data) {
  TRACE(logger, "HttpClientTest start");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app =
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;

  bdf::HttpMessage* msg = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  msg->InitRequest("POST",true);
  msg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "test http_test_client send:hello world.";
  app->GetClientMgr()->Send("http_test_client", msg);

  bdf::HttpMessage* msg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  msg2->InitRequest("POST", true);
  msg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  msg->http_info.body = "test http_test_client send receive:hello world.";
  bdf::EventMessage* msg2_resp =
    app->GetClientMgr()->SendRecieve("http_test_client", msg2, 10);
  if (nullptr != msg2_resp){
    TRACE(logger, "receive msg:" << *msg2_resp);
  }
  return 0;
}

int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::TestClientServerHandler> app;
  app.SetOnAfterStart([&](){
    std::thread t1(&RapidClientTest, &app);
    //std::thread t2(&HttpClientTest, &app);
    t1.join();
    //t2.join();
    return 0;
  });
  return app.Run(argc,argv);
}

#include <sys/prctl.h>
#include <unistd.h>
#include "app/app.h"
#include "test_client_server_handle.h"
#include "message.h"
#include "message_base.h"
#include "client/client_mgr.h"

LOGGER_STATIC_DECL_IMPL(logger, "root");

//valgrind --tool=memcheck --leak-check=full --show-reachable=yes --log-file=test.log ./test_client_server -c ../conf/test_client_server.conf -l ../conf/logger.conf

static bool thread_exit = false;

void RapidClientTestCallBack(bdf::EventMessage* msg) {
  if (msg->status != bdf::MessageBase::kStatusOK){
    TRACE(logger, "call back rapid msg->status:" << (int)(msg->status));
    bdf::MessageFactory::Destroy(msg);
    return;
  }
  bdf::RapidMessage* rapid_msg = dynamic_cast<bdf::RapidMessage*>(msg);
  if (!rapid_msg){
    TRACE(logger, "call back rapid !rapid_msg......");
    bdf::MessageFactory::Destroy(msg);
    return;
  }

  TRACE(logger, "call back rapid msg:" << *msg);
  bdf::MessageFactory::Destroy(msg);
}

void RapidTestSend(bdf::Application<bdf::testserver::TestClientServerHandler>* app) {
  bdf::RapidMessage* rapid_message = BDF_NEW(bdf::RapidMessage);
  rapid_message->body = "test only send rapid_test_client:hello world.";
  app->GetClientMgr()->Send("rapid_test_client", rapid_message);
}

bdf::EventMessage* RapidTestSendRecv(
  bdf::Application<bdf::testserver::TestClientServerHandler>* app) {
  bdf::RapidMessage* rapid_message_2 = BDF_NEW(bdf::RapidMessage);
  rapid_message_2->body = "aaa";
  return app->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2);
}

bool RapidTestInvoe(bdf::Application<bdf::testserver::TestClientServerHandler>* app) {
  bdf::RapidMessage* rapid_message_3 = BDF_NEW(bdf::RapidMessage);
  rapid_message_3->body = "invoke test3";
  bool ret = app->GetClientMgr()->Invoke(
    "rapid_test_client", rapid_message_3, &RapidClientTestCallBack);
  if (!ret) {
    INFO(logger, "ret is false.");
  }
  return ret;
}

int RapidClientTest(void* data) {
  prctl(PR_SET_NAME, "RapidClientTest111111");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app = 
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;
  bdf::ForTest::Inst().SetForTest(true);
  INFO(logger, "RapidClientTestFlag start,time:"<<time(NULL));
  int times = 5000000;
  while (times-- > 0){
    if (0 == times%2500){
      sleep(1);
    }
    //test1
    RapidTestSend(app);
    //test2
    bdf::EventMessage* msg2_resp = RapidTestSendRecv(app);
    if (nullptr == msg2_resp){
      //异步没实现SendRecv,一直返回null
      TRACE(logger, "msg2_resp is null.");
    }else{
      TRACE(logger, "receive msg:" << *msg2_resp);
      if (msg2_resp->status != bdf::EventMessage::kStatusOK) {
        WARN(logger, "time out message");
      }
      bdf::MessageFactory::Destroy(msg2_resp);
    }
    
    //test3
    RapidTestInvoe(app);
    //break;
  }
  INFO(logger, "RapidClientTestFlag stop,time:" << time(NULL));
  return 0;
}

void HttpClientTestCallBack(bdf::EventMessage* msg) {
  if (msg->status != bdf::MessageBase::kStatusOK) {
    TRACE(logger, "call back http msg->status:" << (int)(msg->status));
    bdf::MessageFactory::Destroy(msg);
    return;
  }
  bdf::HttpMessage* http_msg = dynamic_cast<bdf::HttpMessage*>(msg);
  if (!http_msg) {
    TRACE(logger, "call back http !http_msg......");
    bdf::MessageFactory::Destroy(msg);
    return;
  }

  TRACE(logger, "call back http msg:" << *msg);
  bdf::MessageFactory::Destroy(msg);
}

void HttpTestSend(bdf::Application<bdf::testserver::TestClientServerHandler>* app){
  //bdf::HttpMessage* hmsg = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  bdf::HttpMessage* hmsg = BDF_NEW(bdf::HttpMessage);
  hmsg->InitRequest("POST", true);
  hmsg->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg->http_info.url = "/s?a=x&b=y";
  hmsg->http_info.body = "test http_test_client only send:hello world.";
  app->GetClientMgr()->Send("http_test_client", hmsg);
}

bdf::EventMessage* HttpTestSendRecv(
  bdf::Application<bdf::testserver::TestClientServerHandler>* app) {
  //bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  bdf::HttpMessage* hmsg2 = BDF_NEW(bdf::HttpMessage);
  hmsg2->InitRequest("POST", true);
  hmsg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg2->http_info.url = "/snd?a=xx&b=yy";
  hmsg2->http_info.body = "test http_test_client send receive:hello world.";
  return app->GetClientMgr()->SendRecieve("http_test_client", hmsg2);
}

bool HttpTestInvoke(bdf::Application<bdf::testserver::TestClientServerHandler>* app) {
  bdf::HttpMessage* hmsg3 = BDF_NEW(bdf::HttpMessage);
  hmsg3->InitRequest("POST", true);
  hmsg3->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg3->http_info.url = "/snd?a=xx&b=yy";
  hmsg3->http_info.body = "test http_test_client invoke:invoke test.";
  int ret = app->GetClientMgr()->Invoke("http_test_client", hmsg3, &HttpClientTestCallBack);
  if (!ret) {
    TRACE(logger, "ret is false..");
  }
  return ret;
}

int HttpClientTest(void* data) {
  prctl(PR_SET_NAME, "HttpClientTest222222");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app =
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;
  bdf::ForTest::Inst().SetForTest(true);
  INFO(logger, "HttpClientTestFlag start,time:" << time(NULL));
  sleep(1);
  int times = 3000000;
  while (times-- > 0) {
    if (0 == times % 1500) {
      sleep(1);
    }
    //test1
    HttpTestSend(app);
    //test2
    bdf::EventMessage* hmsg2_resp = HttpTestSendRecv(app);
    if (nullptr == hmsg2_resp) {
      //异步没实现SendRecv,一直返回null
      TRACE(logger, "msg2_resp is null.");
    }else{
      TRACE(logger, "receive msg:" << *hmsg2_resp);
      if (hmsg2_resp->status != bdf::EventMessage::kStatusOK) {
        WARN(logger, "time out message");
      }
      bdf::MessageFactory::Destroy(hmsg2_resp);
    }
    
    //test3
    HttpTestInvoke(app);
    //break;
  }
  INFO(logger, "HttpClientTestFlag stop,time:" << time(NULL));
  return 0;
}

int YaceClientTest(void* data) {
  prctl(PR_SET_NAME, "YaceClientTestxxxx");
  bdf::Application<bdf::testserver::TestClientServerHandler>* app =
    (bdf::Application<bdf::testserver::TestClientServerHandler>*)data;
  INFO(logger, "YaceClientTest start,time:" << time(NULL));
  int64_t req_times = 0;
  while (!thread_exit) {
    if (0 == req_times % 66) {
      //66次sleep1毫秒，测试机器使用不到5个核心
      //假如1秒钟有一半时间sleep，那么一秒钟500次
      //实际情况1秒钟不止500次
      usleep(1000);
    }
    //test
    HttpTestInvoke(app);
    //sleep(1);
    RapidTestInvoe(app);
    //sleep(1);
    req_times++;
  }
  INFO(logger, "YaceClientTest stop,time:" << time(NULL));
  return 0;
}

int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::TestClientServerHandler> app;
  app.SetOnStop([]() {
    thread_exit = true;
    return 0;
  });
  app.SetOnAfterStart([&](){
    sleep(1);
    //std::thread t1(&RapidClientTest, &app);
    //std::thread t2(&HttpClientTest, &app);
    std::thread t1(&YaceClientTest, &app);
    t1.join();
    //t2.join();
    return 0;
  });
  return app.AppRun(argc,argv);
}


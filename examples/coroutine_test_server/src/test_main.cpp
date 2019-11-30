#include <unistd.h>
#include "app/app.h"
#include "client/client_mgr.h"
#include "message.h"
#include "app_test_server_handle.h"
#include "task.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

static bool thread_exit = false;

class TaskTest:public bdf::Task{
public:

  void RapidTest(){

    TRACE(logger_, "RapidTest Send.");
    bdf::RapidMessage* rapid_message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
    rapid_message->body = "test rapid_test_client only send:hello world.";
    bdf::AppBase::Get()->GetClientMgr()->Send("rapid_test_client", rapid_message);

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

  void HttpTest() {

    TRACE(logger_, "HttpTest Send.");
    bdf::HttpMessage* hmsg = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
    hmsg->InitRequest("POST", true);
    hmsg->http_info.headers.insert(
      std::pair<std::string, std::string>("Content-Type", "text/html"));
    hmsg->http_info.url = "/oo?a=x&b=y";
    hmsg->http_info.body = "HttpTest only send:hello world.";
    bdf::AppBase::Get()->GetClientMgr()->Send("http_test_client", hmsg);

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

  virtual void OnTask(void* function_data) {
    INFO(logger_, "on task.");
    int times = 1000000;
    while (!thread_exit && times-- > 0) {
      if (times % 2 == 0){
        HttpTest();
      }else{
        RapidTest();
      }
      sleep(3);
      //break;
    }
    INFO(logger_, "exit task.");
  }
};

int main(int argc, char *argv[]){

  TaskTest test_task;

  bdf::Application<bdf::testserver::AppTestServerHandler> app;
  app.SetOnStop([]() {
    thread_exit = true;
    return 0;
  });
  app.SetOnAfterStart([&]() {
    bdf::service::GetIoService().SendTaskToServiceHandle(&test_task);
    return 0;
  });
  app.AppRun(argc,argv);
  return 0;
}

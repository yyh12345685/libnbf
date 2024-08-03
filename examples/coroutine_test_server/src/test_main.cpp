#include <unistd.h>
#include <sys/prctl.h>
#include "app/app.h"
#include "client/client_mgr.h"
#include "service/service_manager.h"
#include "message.h"
#include "app_test_server_handle.h"
#include "task.h"
#include "coroutine/coroutine_manager.h"
#include "coroutine/coroutine_schedule.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

static bool thread_exit = false;

void CreateCoroTest(void* data) {
  std::string* str = (std::string*)(data);
  INFO(logger_, "CreateCoroTest start and exit data:" << *str); // 这里日志没出来，有空再查下原因
  std::cout << "CreateCoroTest start and exit data:" << *str << std::endl;
}

int CoroThreadTest(void* data) {
  prctl(PR_SET_NAME, "CoroThreadTest");
  std::cout << "CoroThreadTest start." << std::endl;
  INFO(logger_, "CoroThreadTest start."); // 这里日志没出来，有空再查下原因
  bdf::CoroutineManager::Instance().Init(nullptr, nullptr);
  bdf::CoroutineSchedule* scheduler = bdf::CoroutineManager::Instance().GetScheduler();
  scheduler->InitCoroSchedule(nullptr, nullptr, 0, 0);
  scheduler->CoroutineStart(&CreateCoroTest, data);
  return 0;
}

class TaskTest:public bdf::Task{
public:
  void RapidTest() {

    TRACE(logger_, "RapidTest Send.");
    bdf::RapidMessage* rapid_message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
    rapid_message->body = "test rapid_test_client only send:hello world.";
    bool send = bdf::AppBase::Get()->GetClientMgr()->Send("rapid_test_client", rapid_message);
    TRACE(logger_, "RapidTest Send msg:" << *rapid_message << ",return:" << send);

    bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
    rapid_message_2->body = "test send recieve rapid";
    TRACE(logger_, "RapidTest send msg:" << rapid_message_2);
    bdf::EventMessage* msg2_resp =
      bdf::AppBase::Get()->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2);
    if (nullptr == msg2_resp) {
      WARN(logger_, "msg2_resp is null.");
      return;
    }
    bdf::RapidMessage* real_msg = dynamic_cast<bdf::RapidMessage*>(msg2_resp);
    TRACE(logger_, "RapidTest receive msg:" << *real_msg);
    bdf::MessageFactory::Destroy(real_msg);
  }

  void HttpTest() {
    bdf::HttpMessage* hmsg = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
    hmsg->InitRequest("POST", true);
    hmsg->http_info.headers.insert(
      std::pair<std::string, std::string>("Content-Type", "text/html"));
    hmsg->http_info.url = "/oo?a=x&b=y";
    hmsg->http_info.body = "HttpTest only send:hello world.";
    bool send = bdf::AppBase::Get()->GetClientMgr()->Send("http_test_client", hmsg);
    TRACE(logger_, "HttpTest Send msg:" << *hmsg << ",return:" << send);

    bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
    hmsg2->InitRequest("POST", true);
    hmsg2->http_info.headers.insert(
      std::pair<std::string, std::string>("Content-Type", "text/html"));
    hmsg2->http_info.url = "/mm?a=xx&b=yy";
    hmsg2->http_info.body = "HttpTest send receive:hello world.";
    TRACE(logger_, "HttpTest SendRecieve send:" << hmsg2);
    bdf::EventMessage* msg2_resp =
      bdf::AppBase::Get()->GetClientMgr()->SendRecieve("http_test_client", hmsg2);
    if (nullptr == msg2_resp) {
      WARN(logger_, "HttpTest msg2_resp is nullptr.");
      return;
    }
    bdf::HttpMessage* hmsg2_resp = dynamic_cast<bdf::HttpMessage*>(msg2_resp);
    TRACE(logger_, "HttpTest receive msg:" << *hmsg2_resp);
    bdf::MessageFactory::Destroy(hmsg2_resp);
  }

  virtual void OnTask(void* function_data) {
    sleep(1);
    INFO(logger_, "on task.");
    int times = 1;
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

// 测试非协程中能否调用SendRecieve[只有协程中能调用，线程中时不能调用的]
void StartTestThread(void* data) {
  prctl(PR_SET_NAME, "StartTestThread");
  sleep(20);
  bdf::HttpMessage* hmsg2 = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  hmsg2->InitRequest("POST", true);
  hmsg2->http_info.headers.insert(
    std::pair<std::string, std::string>("Content-Type", "text/html"));
  hmsg2->http_info.url = "/inthread?a=xx&b=yy";
  hmsg2->http_info.body = "HttpTest send receive:hello world in thread.";
  bdf::EventMessage* msg2_resp =
    bdf::AppBase::Get()->GetClientMgr()->SendRecieve("http_test_client", hmsg2);
  if (nullptr == msg2_resp) {
    WARN(logger_, "StartTestThread msg2_resp is nullptr.");
    return;
  }
  bdf::HttpMessage* hmsg2_resp = dynamic_cast<bdf::HttpMessage*>(msg2_resp);
  TRACE(logger_, "StartTestThread receive msg:" << *hmsg2_resp);
  bdf::MessageFactory::Destroy(hmsg2_resp);
}

int main(int argc, char *argv[]){

  //std::thread t1(&StartTestThread, nullptr);
  std::string test = "hello world.";
  std::thread t2(&CoroThreadTest, &test);

  TaskTest test_task;

  bdf::Application<bdf::testserver::AppTestServerHandler> app;
  app.SetOnStop([]() {
    thread_exit = true;
    return 0;
  });
  app.SetOnAfterStart([&]() {
    bdf::service::GetServiceManager().SendTaskToServiceHandle(&test_task);
    return 0;
  });
  t2.join();
  app.AppRun(argc,argv);
  return 0;
}

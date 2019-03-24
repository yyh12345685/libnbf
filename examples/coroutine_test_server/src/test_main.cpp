#include <unistd.h>
#include "app/app.h"
#include "app/appbase.h"
#include "client/client_mgr.h"
#include "monitor/matrix_scope.h"
#include "message.h"
#include "app_test_server_handle.h"
#include "task.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

class TaskTest:public bdf::Task{
public:
  virtual void OnTask(void* function_data) {
    TRACE(logger_, "on task...");
    int times = 1000000;
    while (times-- > 0) {
      bdf::monitor::MatrixScope matrix_scope(
        "RapidClient", bdf::monitor::MatrixScope::kModeAutoSuccess);
      bdf::RapidMessage* rapid_message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
      rapid_message->body = "test rapid_test_client:hello world.";
      bdf::AppBase::Get()->GetClientMgr()->Send("rapid_test_client", rapid_message);

      bdf::RapidMessage* rapid_message_2 = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
      rapid_message_2->body = "aaa";
      bdf::EventMessage* msg2_resp =
        bdf::AppBase::Get()->GetClientMgr()->SendRecieve("rapid_test_client", rapid_message_2, 10);
      if (nullptr == msg2_resp) {
        TRACE(logger_, "msg2_resp is null.");
        continue;
      }
      bdf::RapidMessage* real_msg = dynamic_cast<bdf::RapidMessage*>(msg2_resp);
      TRACE(logger_, "receive msg:" << *real_msg);
      bdf::MessageFactory::Destroy(real_msg);
      sleep(5);
      //break;
    }
  }
};

int main(int argc, char *argv[]){

  TaskTest test_task;

  bdf::Application<bdf::testserver::AppTestServerHandler> app;

  app.SetOnAfterStart([&]() {
    bdf::service::GetIoService().SendTaskToServiceHandle(&test_task);
    return 0;
  });
  app.Run(argc,argv);
  return 0;
}

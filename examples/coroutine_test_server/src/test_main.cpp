#include "app/app.h"
#include "app_test_server_handle.h"
#include "coroutine/coroutine_context.h"
#include "task.h"

LOGGER_STATIC_DECL_IMPL(logger_, "root");

class TaskTest:public bdf::Task{
public:
  virtual void OnTask(void* function_data) {
    TRACE(logger_, "on task...");
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

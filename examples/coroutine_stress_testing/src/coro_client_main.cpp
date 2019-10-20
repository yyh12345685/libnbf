#include <sys/prctl.h>
#include <unistd.h>
#include "app/app.h"
#include "coro_test_server_handle.h"
#include "service/io_service.h"
#include "client_task_test.h"

LOGGER_STATIC_DECL_IMPL(logger, "root");

int StartTaskThread(void* data) {
  prctl(PR_SET_NAME, "StartTaskThread");
  INFO(logger, "Start TaskThread,time:" << time(NULL));
  uint64_t send_times = 0;
  ClientTaskTest client_test_task;
  while (true){
    bdf::service::GetIoService().SendTaskToServiceHandle(&client_test_task);
    //这种发送方式很容易造成服务端过载,参考test_client_server的方式更佳
    if (0 == (send_times%60000)){
      sleep(1);
    }
    send_times++;
  }
  INFO(logger, "Exit TaskThread,time:" << time(NULL));
  return 0;
}


int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::CoroTestServerHandler> app;
  
  app.SetOnAfterStart([&]() {
    std::thread t1(&StartTaskThread, &app);
    t1.join();
    return 0;
  });
  return app.AppRun(argc, argv);
}


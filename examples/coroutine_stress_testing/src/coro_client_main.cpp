#include <sys/prctl.h>
#include <unistd.h>
#include <cstdlib>
#include <atomic>
#include "app/app.h"
#include "coro_test_server_handle.h"
#include "service/service_manager.h"
#include "client_task_test.h"

LOGGER_STATIC_DECL_IMPL(logger, "root");

static std::atomic<bool> thread_exit(false);

int StartTaskThread(void* data) {
  prctl(PR_SET_NAME, "StartTaskThread");
  INFO(logger, "Start TaskThread,time:" << time(NULL));
  sleep(1);
  uint64_t send_times = 0;
  const char* interval = std::getenv("LIBNBF_STRESS_INTERVAL_US");
  const unsigned interval_us = interval ? std::strtoul(interval, nullptr, 10) : 10;
  ClientTaskTest client_test_task;
  while (!thread_exit){
    bdf::service::GetServiceManager().SendTaskToServiceHandle(&client_test_task);
    //这种发送方式很容易造成服务端过载,可参考test_client_server的方式更佳
    if (interval_us) {
      usleep(interval_us);
    } else if (0 == (send_times%10)){
      usleep(1000);
    }
    send_times++;
  }
  INFO(logger, "Exit TaskThread,time:" << time(NULL));
  return 0;
}


int main(int argc, char *argv[]){
  bdf::Application<bdf::testserver::CoroTestServerHandler> app;
  app.SetOnStop([]() {
    thread_exit = true;
    return 0;
  });
  app.SetOnAfterStart([&]() {
    std::thread t1(&StartTaskThread, &app);
    t1.join();
    return 0;
  });
  return app.AppRun(argc, argv);
}


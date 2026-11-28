#include <atomic>

#include "app/app.h"
#include "invoke_stress_client.h"
#include "service/service_handle.h"

int main(int argc, char* argv[]) {
  std::atomic<bool> stop_requested{false};
  bdf::Application<bdf::ServiceHandler> app;
  app.SetOnStop([&] {
    stop_requested.store(true);
    return 0;
  });
  app.SetOnAfterStart([&] {
    RunInvokeStress(*app.GetClientMgr(), stop_requested);
    return 0;
  });
  return app.AppRun(argc, argv);
}

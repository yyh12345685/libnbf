#include "app/app.h"
#include "coro_test_server_handle.h"

int main(int argc, char *argv[]) {
  bdf::Application<bdf::testserver::CoroTestServerHandler> app;
  return app.AppRun(argc, argv);
}
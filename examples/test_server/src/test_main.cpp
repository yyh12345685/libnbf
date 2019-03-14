#include "app/app.h"
#include "test_server_handle.h"

int main(int argc, char *argv[]) {
  bdf::Application<bdf::testserver::TestHandler> app;
  return app.Run(argc,argv);
}

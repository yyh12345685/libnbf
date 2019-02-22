#include <string>
#include "gtest/gtest.h"
#include "monitor/mem_profile.h"
#include "common/logger.h"
#include "t_struct.h"

using namespace bdf;
using namespace bdf::monitor;

LOGGER_EXTERN_DECL(logger)
LOGGER_IMPL(logger, "root");

int main(int argc, char** argv){
  LOGGER_SYS_INIT("conf/test.conf")
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(MemProfile, testInit) {
  GlobalMatrix::Init("log/mem_test.monitor", 32, 32, 1024 * 128);
}

TEST(MemProfile, memProfile){
  struct timeval tv;
  tv.tv_sec = 0;

  for (int idx = 0; idx < 1000; idx++) {
    std::string * str = BDF_NEW(std::string);
    tv.tv_usec = 20 * 1000;//1000us = 1ms
    select(0, NULL, NULL, NULL, &tv);
    BDF_DELETE(str);
  }

  for (int idx = 0; idx < 1000; idx++) {
    MyTestSt1 * st1 = BDF_NEW(MyTestSt1, std::string("testst1"), 10);
    tv.tv_usec = 20 * 1000;//1000us = 1ms
    select(0, NULL, NULL, NULL, &tv);
    BDF_DELETE(st1);
  }

  for (int idx = 0; idx < 1000; idx++) {
    MyTestSt2<std::string> * st2 = BDF_NEW(MyTestSt2<std::string>, std::string("testst2"));
    tv.tv_usec = 20 * 1000;//1000us = 1ms
    select(0, NULL, NULL, NULL, &tv);
    BDF_DELETE(st2);
  }
  /*
  for (int idx = 0; idx < 1000;idx++) {
    std::string aa("testst3");
    int bb = 10;
    MyTestSt3<std::string, int> * st3 =
      BDF_NEW( ( MyTestSt3 <std::string, int > ), (aa),(bb));
    select(0, NULL, NULL, NULL, &tv);
    BDF_DELETE(st3);
  }

  for (int idx = 0; idx < 1000; idx++) {
    std::string aa("testst");
    MyTestSt<std::string, 10> * st =
      BDF_NEW(  ( MyTestSt <std::string, 10 > ) , (aa));
    select(0, NULL, NULL, NULL, &tv);
    BDF_DELETE(st);
  }
  */
}

TEST(MemProfile, testDestroy) {
  sleep(32);
  GlobalMatrix::Destroy();
}



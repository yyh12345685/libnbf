#include <string>
#include "gtest/gtest.h"
#include "monitor/matrix.h"
#include "monitor/matrix_scope.h"
#include "common/logger.h"

using namespace bdf;
using namespace bdf::monitor;

LOGGER_EXTERN_DECL(logger)
LOGGER_IMPL(logger, "root");

int main(int argc, char** argv){
  srand(time(nullptr));
  LOGGER_SYS_INIT("conf/test.conf")
  
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(TestMatrix, testInit) {
  int ret = GlobalMatrix::Init("log/test.monitor",32, 32, 1024 * 128);
  ASSERT_EQ(0, ret);
}

TEST(TestMatrix, testSet) {
  GlobalMatrix::Instance().Set("test1", 1);
}

TEST(TestMatrix, testAdd) {
  GlobalMatrix::Instance().Add("test2", 1);
}

TEST(TestMatrix, testSub) {
  GlobalMatrix::Instance().Sub("test3", 1);
}

TEST(TestMatrix, testReset) {
  GlobalMatrix::Instance().Reset("test4");
}

TEST(TestMatrix, testBeginEnd1) {
  std::string ok("OK");
  std::string err("ERR");
  std::string test("TEST");
  int idx = 999;
  struct timeval tv;
  while (idx--){
    int64_t tok = GlobalMatrix::Instance().MarkBegin("testmark1");

    tv.tv_sec = 0;
    tv.tv_usec = 50 * (rand() % 1000);//1000us = 1ms
    select(0, nullptr, nullptr, nullptr, &tv);

    if (idx%3==1){
      //TRACE(logger,"-----------OK");
      GlobalMatrix::Instance().MarkEnd(tok, ok);
      //error do not call second argv as string's function
      //GlobalMatrix::Instance().MarkEnd(tok, "OK");
    }else if (idx % 3 == 2) {
      //TRACE(logger, "-----------ERR");
      GlobalMatrix::Instance().MarkEnd(tok, err);
    } else{
      //TRACE(logger, "-----------TEST");
      GlobalMatrix::Instance().MarkEnd(tok, test);
    }
  }
}

TEST(TestMatrix, testBeginEnd2) {
  int idx = 1000;
  struct timeval tv;
  while (idx--) {
    uint64_t tok = GlobalMatrix::Instance().MarkBegin("testmark2");

    tv.tv_sec = 0;
    tv.tv_usec = 50 * (rand() % 1000);//1000us = 1ms
    select(0, nullptr, nullptr, nullptr, &tv);

    if (idx % 2 == 1) {
      GlobalMatrix::Instance().MarkEnd(tok, true);
    } else {
      GlobalMatrix::Instance().MarkEnd(tok, false);
    }
  }
}

TEST(TestMatrix, testScope){
  int idx = 1000;
  struct timeval tv;
  while (idx--) {
    MatrixScope scope("test_scope");
    scope.SetOkay(true);

    tv.tv_sec = 0;
    tv.tv_usec = 50 * (rand() % 1000);//1000us = 1ms
    select(0, nullptr, nullptr, nullptr, &tv);
  }
}

TEST(TestMatrix, testDestroy) {
  sleep(30);
  GlobalMatrix::Destroy();
}

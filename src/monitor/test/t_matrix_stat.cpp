#include "gtest/gtest.h"
#include "monitor/matrix_stat.h"
#include "common/logger.h"

using namespace bdf;
using namespace bdf::monitor;

LOGGER_EXTERN_DECL(logger)
LOGGER_IMPL(logger, "root");

int main(int argc, char** argv){
  LOGGER_SYS_INIT("conf/test.conf")
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(TestMatrixStat, testAvg){
  MatrixStat stat(time(NULL), false);
  ASSERT_EQ(0, stat.GetAvg());

  stat.TimeDistribute(1000);
  stat.TimeDistribute(2000);
  stat.TimeDistribute(3000);
  stat.TimeDistribute(4000);
  stat.TimeDistribute(5000);

  ASSERT_EQ(5, stat.GetCount());
  ASSERT_EQ(15000, stat.GetValue());
  ASSERT_EQ(3000, stat.GetAvg());
}

TEST(TestMatrixStat, testQps) {
  MatrixStat stat(time(NULL) - 5, false);
  ASSERT_EQ(0, stat.GetQps());

  stat.TimeDistribute(1000);
  stat.TimeDistribute(2000);
  stat.TimeDistribute(3000);
  stat.TimeDistribute(4000);
  stat.TimeDistribute(5000);

  ASSERT_EQ(1, stat.GetQps());
}

TEST(TestMatrixStat, testTimeDistribute) {
  MatrixStat stat(time(NULL), false);

  for (int i = 0; i != 50; ++i) {
    stat.TimeDistribute(100);
  }
  for (int i = 0; i != 30; ++i) {
    stat.TimeDistribute(200);
  }
  for (int i = 0; i != 10; ++i) {
    stat.TimeDistribute(300);
  }
  for (int i = 0; i != 5; ++i) {
    stat.TimeDistribute(400);
  }
  for (int i = 0; i != 5; ++i) {
    stat.TimeDistribute(500);
  }

  ASSERT_EQ(100, stat.GetTimeDistribute(0.5));
  ASSERT_EQ(200, stat.GetTimeDistribute(0.8));
  ASSERT_EQ(300, stat.GetTimeDistribute(0.9));
  ASSERT_EQ(400, stat.GetTimeDistribute(0.95));
  ASSERT_EQ(500, stat.GetTimeDistribute(0.99));

  TRACE(logger, "testTimeDistribute:" << stat);
}


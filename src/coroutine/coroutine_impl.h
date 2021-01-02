
#pragma once

#include "coroutine/coroutine.h"
#include "common/logger.h"

namespace bdf{

//struct CoroSchedule;

//change from githup
//https://github.com/cloudwu/coroutine

class CoroutineImpl {
public:
  enum {
    kCoroutineInvalid = 0,
    kCoroutineReady = 1,
    kCoroutineRunning = 2,
    kCoroutineSuspend = 3,
  };

  CoroSchedule* CoroutineInit(int coroutine_size = DEFAULT_COROUTINE);
  void CoroutineClose(CoroSchedule* corotine);

  //创建协程
  int CoroutineNew(CoroSchedule* corotine, CoroutineFunc, void *ud);
  //启动或者恢复挂起的协程
  void CoroutineResume(CoroSchedule* corotine, int id);

  int CoroutineStatus(CoroSchedule* corotine, int id);
  int CoroutineRunning(CoroSchedule* corotine);

  //挂起运行中的协程
  void CoroutineYield(CoroSchedule* corotine);

  CoroutineActor* GetCoroutineCtx(CoroSchedule * corotine,int id);

protected:
  void SaveStack(CoroutineActor* coctx, char *top);

private:
  LOGGER_CLASS_DECL(logger_);
};

}

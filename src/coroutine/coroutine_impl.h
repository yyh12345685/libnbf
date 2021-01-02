
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

  //����Э��
  int CoroutineNew(CoroSchedule* corotine, CoroutineFunc, void *ud);
  //�������߻ָ������Э��
  void CoroutineResume(CoroSchedule* corotine, int id);

  int CoroutineStatus(CoroSchedule* corotine, int id);
  int CoroutineRunning(CoroSchedule* corotine);

  //���������е�Э��
  void CoroutineYield(CoroSchedule* corotine);

  CoroutineActor* GetCoroutineCtx(CoroSchedule * corotine,int id);

protected:
  void SaveStack(CoroutineActor* coctx, char *top);

private:
  LOGGER_CLASS_DECL(logger_);
};

}

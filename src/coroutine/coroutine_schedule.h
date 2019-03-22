
#pragma once

#include "coroutine/coroutine.h"
#include "common/logger.h"

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 32

namespace bdf{

struct CoroutineActor;

class CoroutineSchedule {
public:
  enum {
    kCoroutineInvalid = 0,
    kCoroutineReady = 1,
    kCoroutineRunning = 2,
    kCoroutineSuspend = 3,
  };

  CoroutineActor* CoroutineInit();
  void CoroutineClose(CoroutineActor*);

  int CoroutineNew(CoroutineActor*, CoroutineFunc, void *ud);
  void CoroutineResume(CoroutineActor*, int id);
  int CoroutineStatus(CoroutineActor*, int id);
  int CoroutineRunning(CoroutineActor*);
  void CoroutineYield(CoroutineActor*);

private:
  LOGGER_CLASS_DECL(logger_);
};

}

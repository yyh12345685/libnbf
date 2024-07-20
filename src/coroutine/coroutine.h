#pragma once

#include <queue>

namespace bdf {

 typedef void (*CoroutineFunc)(void*);
#define STACK_SIZE (256*1024)
#define DEFAULT_COROUTINE 512

 class CoroContext;

class Coroutine {
public:
  enum {
    kCoroutineInvalid = 0,
    kCoroutineReady = 1,
    kCoroutineRunning = 2,
    kCoroutineSuspend = 3,
  };

  enum {
    kCoroutineTypeC = 0, 
  };

  virtual int CoroutineSize() { return 0; }
  virtual bool CoroutineInit(
    CoroutineFunc func, void* data, 
    std::queue<CoroContext*>& free_list, int coroutine_size = DEFAULT_COROUTINE) {
    return false;
  }
  virtual void Release() {}

  virtual CoroContext* GetCurrentCoroutine() { return nullptr; }

  virtual int CoroutineStatus(CoroContext* coro) { return 0; }

  //启动或者恢复挂起的协程
  virtual bool CoroutineResume(CoroContext* coro) { return false; }

  //挂起运行中的协程
  virtual void CoroutineYield() {}
};

} 

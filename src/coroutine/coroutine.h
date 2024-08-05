#pragma once

#include <unordered_set>

namespace bdf {

 typedef void (*CoroutineFunc)(void*);
#define DEFAULT_STACK_SIZE (128*1024)
#define DEFAULT_COROUTINE 256

 class CoroContext;
 class CoroRelease;

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
    CoroutineFunc func, void* data, CoroRelease* release,
    std::unordered_set<CoroContext*>& free_list, int coroutine_size = DEFAULT_COROUTINE, int stack_size = 0) {
    return false;
  }
  virtual void Release() {}

  // 创建协程
  virtual CoroContext* CoroutineNew(CoroutineFunc, void *ud, CoroRelease* release, int stack_size = 0) { return nullptr; }

  // 获取当前运行中的协程
  virtual CoroContext* GetCurrentCoroutine() { return nullptr; }

  virtual int CoroutineStatus(CoroContext* coro) { return 0; }

  // 启动协程
  virtual void CoroutineStart(CoroContext* coro, CoroutineFunc func = nullptr, void* data = nullptr) {}

  // 启动或者恢复挂起的协程
  virtual bool CoroutineResume(CoroContext* coro) { return false; }

  // 挂起运行中的协程
  virtual void CoroutineYield() {}
};

} 

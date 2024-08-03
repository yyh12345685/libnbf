#pragma once

#include <unordered_set>

namespace bdf {

 typedef void (*CoroutineFunc)(void*);
#define DEFAULT_STACK_SIZE (128*1024)
#define DEFAULT_COROUTINE 256

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
    std::unordered_set<CoroContext*>& free_list, int coroutine_size = DEFAULT_COROUTINE, int stack_size = 0) {
    return false;
  }
  virtual void Release() {}

  // ����Э��
  virtual CoroContext* CoroutineNew(CoroutineFunc, void *ud, int stack_size = 0) { return nullptr; }

  // ��ȡ��ǰ�����е�Э��
  virtual CoroContext* GetCurrentCoroutine() { return nullptr; }

  virtual int CoroutineStatus(CoroContext* coro) { return 0; }

  //����Э��
  virtual void CoroutineStart(CoroContext* coro, CoroutineFunc func = nullptr, void* data = nullptr) {}

  //�������߻ָ������Э��
  virtual bool CoroutineResume(CoroContext* coro) { return false; }

  //���������е�Э��
  virtual void CoroutineYield() {}
};

} 

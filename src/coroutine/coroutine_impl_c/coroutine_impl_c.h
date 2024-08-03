
#pragma once

#include "context.h"
#include "coroutine/coroutine.h"
#include "common/logger.h"

namespace bdf{

class CoroContext;
class CoroContextc;
class CoroContextList;

//change from githup
//https://github.com/cloudwu/coroutine

//使用c语言的函数实现的协程
class CoroutineImplc : public Coroutine {
public:

  virtual bool CoroutineInit(
    CoroutineFunc func, void* data, 
    std::unordered_set<CoroContext*>& free_list, int coroutine_size = DEFAULT_COROUTINE, int stack_size = 0);

  virtual void Release();

  virtual int CoroutineSize();

  //创建协程
  virtual CoroContext* CoroutineNew(CoroutineFunc, void *ud, int stack_size = 0);

  //启动协程
  virtual void CoroutineStart(CoroContext* coro, CoroutineFunc func = nullptr, void* data = nullptr);

  //启动协程/恢复挂起的协程
  virtual bool CoroutineResume(CoroContext* coro);

  virtual int CoroutineStatus(CoroContext* coctx);

  //挂起运行中的协程
  virtual void CoroutineYield();

  virtual CoroContext* GetCurrentCoroutine();

protected:
  //void SaveStack(CoroContextc* coctx, char *top);

private:
  CoroContextList* coro_ls_;
  LOGGER_CLASS_DECL(logger_);
};

}

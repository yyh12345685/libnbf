#pragma once

#include <vector>
#include <unordered_set>
#include <queue>
#include <mutex>
#include "common/logger.h"
#include "common/common.h"
#include "coroutine.h"

namespace bdf {

class CoroContext;

class CoroutineSchedule {
public:
  ~CoroutineSchedule();

  void InitCoroSchedule(CoroutineFunc func, void* data, int coroutine_size, int coroutine_type);

  // 支持使用InitCoroSchedule中的值，或者新传入的值
  void CoroutineStart(CoroutineFunc func = nullptr, void* data = nullptr);

  // 准备接收数据挂起当前运行的协程
  void ReceiveCoroutineYield();

  //挂起当前运行的协程
  void CoroutineYield();

  bool CoroutineYieldToActive(CoroContext* coro);

  CoroContext* GetCurrentCoroutine();

  //启动协程或者恢复挂起的协程
  bool ScCoroutineResume(CoroContext* coro);

  // 尝试切换到一个已经接收数据就绪的协程
  bool CoroutineResumeActive();

  CoroContext* GetAvailableCoro();

  void ProcessDebug();
protected:
  int CoroutineStatus();

  int CoroutineSize() {
    return coro_impl_->CoroutineSize();
  }

  void CoroutineStart(CoroContext* coro, CoroutineFunc func = nullptr, void* data = nullptr);

private:

  std::unordered_set<CoroContext*> free_list_; //空闲可用的协程/或者随时可以resume进去的

  // 任务结束的协程，客户端答复了，或者超时了，待处理的协程
  std::queue<CoroContext*> active_coro_list_;

  Coroutine* coro_impl_;

  CoroutineFunc coro_func_;
  void* coro_data_;

  //for debug begin
  //struct  YieldTimeDebug{
  //  bool is_yield = false;
  //  time_t yield_time = 0;
  //};
  //std::vector<YieldTimeDebug> yield_time_debug_;
  //for debug end

  LOGGER_CLASS_DECL(logger_);
};

}

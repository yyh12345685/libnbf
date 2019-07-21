#pragma once

#include <vector>
#include <set>
#include <unordered_set>
#include "coroutine/coroutine_impl.h"

namespace bdf {

class CoroutineActor;

class CoroutineSchedule {
public:
  ~CoroutineSchedule();

  void InitCoroSchedule(CoroutineFunc func, void* data);

  void ProcessCoroutine(int coro_id);

  int GetAvailableCoroId();

  CoroutineActor* GetCoroutineCtx(int id);

  //启动协程或者恢复挂起的协程
  void CoroutineResume(int id);
  //获取正在运行的协程id
  int GetRunningId();
protected:
  int CoroutineStatus(int id);

  //挂起运行中的协程
  void CoroutineYield();

private:

  std::vector<int> all_coro_list_;//全部
  std::set<int> available_coro_list_;//可用的
  std::unordered_set<int> task_coro_list_;//任务中的

  CoroutineImpl coro_impl_;
  CoroSchedule* coro_sche_;

  LOGGER_CLASS_DECL(logger_);
};

}
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

  void InitCoroSchedule(CoroutineFunc func, void* data,int coroutine_size);

  //挂起当前运行的协程
  void CoroutineYield();

  bool CoroutineYield(CoroContext* coro);

  CoroContext* GetCurrentCoroutine();

  //启动协程或者恢复挂起的协程
  bool CoroutineResume(CoroContext* coro);

  CoroContext* GetAvailableCoro();

  void ProcessDebug();
protected:
  int CoroutineStatus();

  int CoroutineSize(){
    return coro_impl_.CoroutineSize();
  }

private:

  std::queue<CoroContext*> free_list_; //空闲可用的协程

  std::queue<CoroContext*> running_list_; //正在运行或者即将运行的协程

  //任务结束的协程，客户端答复了，或者超时了
  std::queue<int> active_coro_list_;

  Coroutine coro_impl_;

  //for debug begin
  //struct  YieldTimeDebug{
  //  bool is_yield = false;
  //  time_t yield_time = 0;
  //};
  //std::vector<YieldTimeDebug> yield_time_debug_;
  //for debug end

  LOGGER_CLASS_DECL(logger_);
};

class CoroutineID{
public:
  static CoroutineID& GetInst() {
    static CoroutineID inst;
    return inst;
  }
  ~CoroutineID(){
  }

  void InitMaxIds(int max_coro_id);

  std::vector<int>& GetAllCoroIds() { 
    return all_coro_ids_tmp_;
  }
private:
  std::mutex lock_;

  std::vector<int> all_coro_ids_tmp_;
  DISALLOW_COPY_AND_ASSIGN(CoroutineID)
  CoroutineID(){
  }
};

}

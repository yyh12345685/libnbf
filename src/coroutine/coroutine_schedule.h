#pragma once

#include <vector>
//#include <set>
#include <unordered_set>
#include <queue>
#include <mutex>
#include "common/common.h"
#include "coroutine/coroutine_impl.h"

namespace bdf {

class CoroutineActor;

class CoroutineSchedule {
public:
  ~CoroutineSchedule();

  void InitCoroSchedule(CoroutineFunc func, void* data,int coroutine_size);

  void CoroutineYield(int coro_id);

  //���������е�Э��
  void CoroutineYield();

  bool CoroutineYieldToActive(int coro_id);

  bool AfterYieldToAvailable(int coro_id);

  int GetAvailableCoroId();

  CoroutineActor* GetCoroutineCtx(int id);

  //����Э�̻��߻ָ��з��ص�Э��
  bool CoroutineResumeActive();
  //����Э�̻��߻ָ������Э��
  void CoroutineResume(int id);
  //��ȡ�������е�Э��id
  int GetRunningId();

  void ProcessDebug();
protected:
  int CoroutineStatus(int id);

  int CoroutineSize(){
    return coro_sche_->cap;
  }

private:
  int AddNewCoroutine();
  CoroutineFunc func_ = nullptr;
  void* data_ = nullptr;
  //int max_coro_id_ = -1;

private:

  std::vector<int> all_coro_list_;//ȫ��

  std::unordered_set<int> available_coro_list_;//���õ�

  //���������Э�̣��ͻ��˴��ˣ����߳�ʱ��
  std::queue<int> active_coro_list_;

  CoroutineImpl coro_impl_;
  CoroSchedule* coro_sche_;

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
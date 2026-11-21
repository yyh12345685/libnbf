
#pragma once

#include <queue>
#include "common/timer_mgr.h"
#include "message_base.h"
#include "service/service_handle.h"

namespace bdf { 

class HandleData;

class CoroutineServiceHandler : 
  public ServiceHandler{
public:
  CoroutineServiceHandler():debug_time_(0){
  }
  virtual ~CoroutineServiceHandler(){
    while (!pending_.empty()) {
      MessageFactory::Destroy(pending_.front());
      pending_.pop();
    }
  }

  virtual void Run(HandleData* data);

  virtual ServiceHandler* Clone() {
    return new CoroutineServiceHandler();
  }

protected:
  friend class CoroTimer;
  virtual void OnTimer(void* function_data){}

  virtual void OnTimerCoro(void* function_data, int& coro_id);
private:
  LOGGER_CLASS_DECL(logger_);

  static void ProcessCoroutine(void* data);
  void ProcessTimer();
  void Process(HandleData* data);

  void ProcessTask(HandleData* data);

  void ProcessMessageHandle(std::queue<EventMessage*>& msg_list);

  // Messages taken from HandleData but not handled yet. A member (one per
  // handler thread) rather than a local of the running coroutine: when a
  // handler yields in SendRecieve, the next coroutine keeps draining it.
  std::queue<EventMessage*> pending_;
  size_t max_coroutine_size_ = 10000;

  void ProcessClientItem(EventMessage* msg);

  TimerMgr time_mgr_;

  //for debug
  time_t debug_time_;
};


class CoroTimer : public TimerMgrBase {
public:
  CoroTimer(CoroutineServiceHandler* handle, int coro_id = -1):
    service_handle_(handle), coro_id_(coro_id){
    // The callback reads the id through timer_data_; keep it in the timer
    // itself (it pointed into CoroutineID's fixed 100000-entry table, out of
    // bounds once more coroutines were created).
    timer_data_ = &coro_id_;
  }

  virtual void OnTimer(void* timer_data, uint64_t time_id);
  virtual ~CoroTimer() { }
protected:
  CoroutineServiceHandler* service_handle_;
  int coro_id_;
};

}

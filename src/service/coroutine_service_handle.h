
#pragma once

#include <queue>
#include <mutex>
#include "common/timer_mgr.h"
#include "message_base.h"
#include "service/service_handle.h"

namespace bdf { 

class HandleData;
class CoroContext;

class CoroutineServiceHandler : public ServiceHandler {
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

  //void AddTimeOutFromClient(CoroContext* msg_coro);

protected:
  friend class CoroTimer;
  virtual void OnTimer(void* function_data){}

  virtual void OnTimerCoro(void* function_data);
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

  //std::queue<CoroContext*> time_out_coro_;
  //std::mutex time_out_lock_;
};


class CoroTimer : public TimerMgrBase {
public:
  CoroTimer(CoroutineServiceHandler* handle, CoroContext* coro)
    : service_handle_(handle) {
    timer_data_ = coro;
  }

  virtual void OnTimer(void* timer_data, uint64_t time_id);
  virtual ~CoroTimer() { }
protected:
  CoroutineServiceHandler* service_handle_;

};

}

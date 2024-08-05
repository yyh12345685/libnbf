
#pragma once

#include <queue>
#include <mutex>
#include "common/timer_mgr.h"
#include "service/service_handle.h"

namespace bdf { 

class HandleData;
class CoroContext;

class CoroutineServiceHandler : public ServiceHandler {
public:
  CoroutineServiceHandler():debug_time_(0){
  }
  virtual ~CoroutineServiceHandler(){
  }

  virtual void Run(HandleData* data);

  virtual ServiceHandler* Clone() {
    return new CoroutineServiceHandler();
  }

  void AddTimeOutFromClient(CoroContext* msg_coro);

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

  void ProcessClientItem(EventMessage* msg);

  TimerMgr time_mgr_;

  //for debug
  time_t debug_time_;

  std::queue<CoroContext*> time_out_coro_;
  std::mutex time_out_lock_;
};


class CoroTimer : public TimerMgrBase {
public:
  CoroTimer(CoroutineServiceHandler* handle):
    service_handle_(handle){
  }

  virtual void OnTimer(void* timer_data, uint64_t time_id);
  virtual ~CoroTimer() { }
protected:
  CoroutineServiceHandler* service_handle_;
};

}

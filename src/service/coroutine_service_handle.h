
#pragma once

#include <queue>
#include "event/timer/timer.h"
#include "event/timer/timer_base.h"
#include "service/service_handle.h"

namespace bdf { 

class HandleData;

class CoroutineServiceHandler : 
  public ServiceHandler, public OnTimerBase {
public:
  CoroutineServiceHandler():
    current_coroutine_id_(-1)/*,
    current_timer_coroutine_id_(-1),
    current_task_coroutine_id_(-1)*/{
  }
  virtual ~CoroutineServiceHandler(){
  }

  virtual void Run(HandleData* data);

  virtual ServiceHandler* Clone() {
    return new CoroutineServiceHandler();
  }

protected:
  virtual void OnTimer(void* function_data);
private:
  LOGGER_CLASS_DECL(logger_);

  static void ProcessCoroutine(void* data);
  void ProcessTimer();
  void Process(HandleData* data);

  void ProcessTask(HandleData* data);

  void ProcessMessageHandle(std::queue<EventMessage*>& msg_list);

  void ProcessClientItem(EventMessage* msg);

  void Resume(int coroutine_id);

  int current_coroutine_id_;
  //int current_timer_coroutine_id_;
  //int current_task_coroutine_id_;

  Timer timer_;
};

}

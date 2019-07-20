
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_context.h"
#include "event/timer/timer_base.h"
#include "event/timer/timer.h"
#include "service/coroutine_service_handle.h"
#include "message.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActor);

EventMessage* CoroutineActor::RecieveMessage(uint32_t timeout_ms){
  TRACE(logger_, "CoroutineActor::RecieveMessage,timeout_ms:"
    << timeout_ms<<",msg_list_:"<<msg_list_.size());
  uint64_t time_id = 0;
  if (msg_list_.empty()){
    if (timeout_ms) {
      CoroutineServiceHandler* hdl = CoroutineContext::Instance().GetServiceHandler();
      TimerData data;
      data.function_data = nullptr;
      data.time_out_ms = timeout_ms;
      data.time_proc = hdl;
      time_id = CoroutineContext::Instance().GetTimer()->AddTimer(timeout_ms, data);
    }
    CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
    TRACE(logger_, "RecieveMessage CoroutineYield:" << this);
    scheduler->CoroutineYield(this);
  }
  is_waiting_ = false;
  waiting_id_ = -1;
  if (0!=time_id){
    CoroutineContext::Instance().GetTimer()->DelTimer(time_id);
  }
  
  if (!msg_list_.empty()) {
    EventMessage* message = msg_list_.front();
    msg_list_.pop_front();
    return message;
  } else {
    return nullptr;
  }
}

bool CoroutineActor::SendMessage(EventMessage* message){
  TRACE(logger_, "SendMessage,is_waiting_" << is_waiting_
    << ",sequence id:" << message->sequence_id << ",waiting_id:" << waiting_id_
    << ",msg_list_size:" << msg_list_.size());
  if (is_waiting_ && message->sequence_id != waiting_id_) {
    WARN(logger_, "sequence_id mismatch:" << waiting_id_ << ",msg:" << *message);
    return false;
  }
  
  msg_list_.emplace_back(message);
  return true;
}

}

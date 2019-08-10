
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_context.h"
#include "event/timer/timer_base.h"
#include "event/timer/timer.h"
#include "service/coroutine_service_handle.h"
#include "message.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActor);

EventMessage* CoroutineActor::RecieveMessage(EventMessage* message,uint32_t timeout_ms){
  TRACE(logger_, "CoroutineActor::RecieveMessage,timeout_ms:"
    << timeout_ms<<",msg_list_:"<<msg_list_.size());
  uint64_t time_id = 0;
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  if (msg_list_.empty()){
    if (timeout_ms) {
      CoroutineServiceHandler* hdl = CoroutineContext::Instance().GetServiceHandler();
      TimerData data;
      data.function_data = &message->coroutine_id;
      //data.time_out_ms = timeout_ms;
      data.time_proc = hdl;
      //这里定时器增加5ms，因为在async_client_connect和sync_sequence也有个定时器，那个时间是标准的
      //需要connect中的定时器先触发，否则回有bug，所以这里先简单解决，多延迟5ms
      //如果协程中的定时器先触发，会导致消息顺序不对
      int real_time_out = timeout_ms + 5;
      time_id = CoroutineContext::Instance().GetTimer()->AddTimer(real_time_out, data);
    }
    TRACE(logger_, "RecieveMessage CoroutineYield:" << this);
    scheduler->CoroutineYield(message->coroutine_id);
  }
  is_waiting_ = false;
  waiting_id_ = -1;
  if (0!=time_id){
    CoroutineContext::Instance().GetTimer()->DelTimer(time_id);
  }
  
  if (!msg_list_.empty()) {
    EventMessage* msg = msg_list_.front();
    msg_list_.pop_front();
    return msg;
  } else {
    INFO(logger_, "maybe time out:"<< timeout_ms <<",coro id:" << 
      message->coroutine_id  << ",running id:" << scheduler ->GetRunningId()<<",prt:"<< this);
    return nullptr;
  }
}

bool CoroutineActor::SendMessage(EventMessage* message){
  TRACE(logger_, "SendMessage,is_waiting_" << is_waiting_
    << ",sequence id:" << message->sequence_id << ",waiting_id:" << waiting_id_
    << ",msg_list_size:" << msg_list_.size());
  if (is_waiting_ && message->sequence_id != waiting_id_) {
    INFO(logger_, "sequence_id mismatch:" << waiting_id_ << ",msg:" << *message);
    //同步的协议超时就关闭了连接，不会来到这里
    //rapid协议超时之后返回的，两个定时器的原因
    if (is_waiting_> message->sequence_id){
      MessageFactory::Destroy(message);
    }
    return false;
  }
  
  msg_list_.emplace_back(message);
  waiting_id_ = -1;
  return true;
}

}

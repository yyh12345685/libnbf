
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_context.h"
#include "service/coroutine_service_handle.h"
#include "message.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActor);

CoroutineActor::CoroutineActor() :
  is_waiting_(false),
  waiting_id_(-1) {
  msg_list_.clear();
}

EventMessage* CoroutineActor::RecieveMessage(EventMessage* message,uint32_t timeout_ms){
  TRACE(logger_, "CoroutineActor::RecieveMessage,timeout_ms:"
    << timeout_ms<<",msg_list_:"<<msg_list_.size());
  uint64_t time_id = 0;
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  uint64_t seq_id_send_tmp = message->sequence_id;
  int cur_coro_id = CoroutineContext::GetCurCoroutineId();
  int coro_id_tmp = message->coroutine_id;
  if (cur_coro_id != coro_id_tmp){
    INFO(logger_, "may be send failed in io handle, cur_coro_id:"
      << cur_coro_id <<",coro_id_tmp:"<< coro_id_tmp);
    return nullptr;
  }
  CoroTimer* tim = nullptr;
  if (likely(msg_list_.empty())){
    CoroutineServiceHandler* hdl = CoroutineContext::Instance().GetServiceHandler();
    tim = BDF_NEW (CoroTimer,hdl);
    tim->timer_data_ = &(CoroutineID::GetInst().GetAllCoroIds()[cur_coro_id]);
    time_id = CoroutineContext::Instance().GetTimerMgr()->AddTimer(tim, timeout_ms);
    TRACE(logger_, "ThreadId:"<< ThreadId::Get()
      <<",RecieveMessage CoroutineYield:" << this<<",msg"<< *message);
    scheduler->CoroutineYield(cur_coro_id);
    scheduler->AfterYieldToAvailable(cur_coro_id);
  }else{
    WARN(logger_, "tid:" << ThreadId::Get() << ",msg_list_ size is:" << msg_list_.size());
  }
  is_waiting_ = false;
  waiting_id_ = -1;
  if (0 != time_id && nullptr != tim){
    CoroutineContext::Instance().GetTimerMgr()->DelTimer(time_id);
    BDF_DELETE(tim);
  }
  
  if (likely(!msg_list_.empty())) {
    EventMessage* msg = msg_list_.front();
    msg_list_.pop_front();
    if (seq_id_send_tmp != msg->sequence_id) {
      //异常情况，理论不存在
      WARN(logger_, "tid:" << ThreadId::Get() << ",ccoro id:"<< cur_coro_id
        << ",send message seq_id:" << seq_id_send_tmp << ",resp message:" << *msg);
      MessageFactory::Destroy(msg);
      return nullptr;
    }
    return msg;
  } else {
    /*INFO*/TRACE(logger_, "maybe time out:"<< timeout_ms <<",coro id:" <<
      coro_id_tmp << ",running id:" << scheduler ->GetRunningId()<<",prt:"<< this);
    return nullptr;
  }
}

bool CoroutineActor::SendMessage(EventMessage* message){
  TRACE(logger_, "SendMessage,is_waiting_" << is_waiting_
    << ",sequence id:" << message->sequence_id << ",waiting_id:" << waiting_id_
    << ",msg_list_size:" << msg_list_.size());
  if (unlikely(!is_waiting_ || (is_waiting_ && message->sequence_id != waiting_id_))) {
    /*INFO*/TRACE(logger_, "sequence_id mismatch:" << waiting_id_
      << ",msg_list_size:"<< msg_list_ .size()<<",msg:" << *message);
    //同步的协议超时就关闭了连接，不会来到这里,到外面处理
    //rapid协议超时之后返回的，两个定时器的原因
    return false;
  }

  TRACE(logger_, "ThreadId:" << ThreadId::Get() << ",respose message:" << *message);
  msg_list_.emplace_back(message);
  waiting_id_ = -1;
  return true;
}

}

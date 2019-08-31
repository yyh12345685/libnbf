
#include "coroutine/coroutine_actor.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_context.h"
#include "event/timer/timer_base.h"
#include "event/timer/timer.h"
#include "service/coroutine_service_handle.h"
#include "message.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActor);

//for not release
//////////////////////////////////////////////////////////////////////////
//用来在不同的协程中传递协程id，直接一个协程中的变量，无法传递到另外一个协程
static std::vector<int> g_coro_id_tmp;
static void InitCoroTest(){
  if (g_coro_id_tmp.size() != 0){
    return;
  }
  for (int idx = 0; idx < 100000;idx++) {
    g_coro_id_tmp.emplace_back(idx);
  }
}
//////////////////////////////////////////////////////////////////////////

CoroutineActor::CoroutineActor() :
  is_waiting_(false),
  waiting_id_(-1) {
  msg_list_.clear();
  InitCoroTest();
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
  if (msg_list_.empty()){
    if (timeout_ms) {
      CoroutineServiceHandler* hdl = CoroutineContext::Instance().GetServiceHandler();
      TimerData data;
      data.function_data = &g_coro_id_tmp[cur_coro_id]/*&cur_coro_id*/;
      data.time_proc = hdl;
      //这里定时器增加3ms，因为在async_client_connect和sync_sequence也有个定时器，那个时间是标准的
      //理论上需要connect中的定时器先触发，如果协程中的定时器先触发
      //connect中定时器后触发会导致SendMessage中waiting_id_和message的sequence_id差异越大
      int real_time_out = timeout_ms + 2;
      time_id = CoroutineContext::Instance().GetTimer()->AddTimer(real_time_out, data);
    }
    TRACE(logger_, "ThreadId:"<< ThreadId::Get()
      <<",RecieveMessage CoroutineYield:" << this<<",msg"<< *message);
    scheduler->CoroutineYield(cur_coro_id);
    scheduler->AfterYieldToAvailable(cur_coro_id);
  }
  is_waiting_ = false;
  waiting_id_ = -1;
  if (0 != time_id){
    CoroutineContext::Instance().GetTimer()->DelTimer(time_id);
  }
  
  if (!msg_list_.empty()) {
    EventMessage* msg = msg_list_.front();
    msg_list_.pop_front();
    if (seq_id_send_tmp != msg->sequence_id) {
      WARN(logger_, "ThreadId:" << ThreadId::Get()
        << ",send message seq_id:" << seq_id_send_tmp << ",resp message:" << *msg);
    }
    return msg;
  } else {
    INFO(logger_, "maybe time out:"<< timeout_ms <<",coro id:" << 
      coro_id_tmp << ",running id:" << scheduler ->GetRunningId()<<",prt:"<< this);
    return nullptr;
  }
}

bool CoroutineActor::SendMessage(EventMessage* message){
  TRACE(logger_, "SendMessage,is_waiting_" << is_waiting_
    << ",sequence id:" << message->sequence_id << ",waiting_id:" << waiting_id_
    << ",msg_list_size:" << msg_list_.size());
  if (is_waiting_ && message->sequence_id != waiting_id_) {
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

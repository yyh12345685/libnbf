
#include "coroutine_actor_c.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_manager.h"
#include "service/coroutine_service_handle.h"
#include "message.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActorc);

CoroutineActorc::CoroutineActorc() :
  is_waiting_(false),
  waiting_id_(-1) {
  msg_list_.clear();
}

EventMessage* CoroutineActorc::RecieveMessage(EventMessage* message, uint32_t timeout_ms){
  TRACE(logger_, "CoroutineActorc::RecieveMessage,timeout_ms:"
    << timeout_ms<<",msg_list_:"<<msg_list_.size());
  // uint64_t time_id = 0;
  message->handle_svr =  CoroutineManager::Instance().GetServiceHandler();
  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  uint64_t seq_id_send_tmp = message->sequence_id;
  CoroContext* msg_coro = message->msg_coro;
  if (msg_coro == nullptr) {
    WARN(logger_, "may be send failed in io handle, message:" << *message);
    return nullptr;
  }
  //CoroTimer* tim = nullptr;
  if (likely(msg_list_.empty())){
    //CoroutineServiceHandler* hdl = CoroutineManager::Instance().GetServiceHandler();
    //tim = BDF_NEW (CoroTimer, hdl);
    //tim->timer_data_ = msg_coro;
    // 该定时器是为了请求超时之后，协程可以切换回来
    //time_id = CoroutineManager::Instance().GetTimerMgr()->AddTimer(tim, timeout_ms);
    //TRACE(logger_, "ThreadId:"<< ThreadId::Get()
    //  <<",RecieveMessage CoroutineYield:" << msg_coro <<",msg"<< *message);
    scheduler->ReceiveCoroutineYield(); // 在协程里面，切出当前协程，切回来的条件是要么收到消息，要么超时
  }else{
    WARN(logger_, "tid:" << ThreadId::Get() << ",msg_list_ size is:" << msg_list_.size());
  }
  is_waiting_ = false;
  waiting_id_ = -1;
  /*if (0 != time_id && nullptr != tim){
    CoroutineManager::Instance().GetTimerMgr()->DelTimer(time_id);
    BDF_DELETE(tim);
  }*/
  
  while(!msg_list_.empty()) {
    EventMessage* msg = msg_list_.front();
    msg_list_.pop_front();
    if (seq_id_send_tmp != msg->sequence_id) {
      //可能是超时的返回，需要把seqid不等的都删掉
      WARN(logger_, "tid:" << ThreadId::Get()<< ",send message seq_id:" 
        << seq_id_send_tmp << ",resp message:" << *msg);
      MessageFactory::Destroy(msg);
      continue;
    }
    return msg;
  }

  TRACE(logger_, "maybe time out:"<< timeout_ms << ",running coro:" 
    << scheduler ->GetCurrentCoroutine() << ",prt:"<< this);
  return nullptr;
}

bool CoroutineActorc::SendMessage(EventMessage* message){
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

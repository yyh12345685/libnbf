#include "net/sync_sequence.h"
#include "message.h"
#include "common/time.h"
#include "net/sync_client_connect.h"
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "common/string_util.h"
#include "service/coroutine_service_handle.h"
#include "coroutine/coroutine_manager.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

SyncSequence::SyncSequence(
  SyncClientConnect* sync_client_con,uint32_t timeout_in_ms):
  sync_client_con_(sync_client_con),
  timeout_ms_(timeout_in_ms){
}

SyncSequence::~SyncSequence(){
}

uint64_t SyncSequence::StartTimer(EventMessage* message) {
  TimerData td;
  td.time_out_ms = timeout_ms_;
  td.time_proc = this;
  td.function_data = message;
  //这里只是用来驱动超时检测,3ms之后执行OnTimer
  int thread_id = sync_client_con_->GetRegisterThreadId();
  return ServiceManager::GetInstance().GetNetThreadManager()->StartTimer(td, thread_id);
}

int SyncSequence::Put(EventMessage* message) {
  lock_.lock();
  list_.emplace_back(message);
  lock_.unlock();

  message->timer_out_id = StartTimer(message);
  TRACE(logger_, "start timer id:" << message->timer_out_id);
  return 0;
}

EventMessage* SyncSequence::Get() {
  lock_.lock();
  if (0 == list_.size()) {
    lock_.unlock();
    return nullptr;
  }
  // 这里默认同一个连接上的消息遵循先发送，先返回，后发送后返回
  EventMessage* fired = list_.front();
  list_.pop_front();
  lock_.unlock();

  if (nullptr != fired && fired->timer_out_id > 0) {
    int thread_id = sync_client_con_->GetRegisterThreadId();
    ServiceManager::GetInstance().GetNetThreadManager()->CancelTimer(
      fired->timer_out_id, thread_id);
    TRACE(logger_, "have response cancel timer id:" << fired->timer_out_id);
  } else {
    WARN(logger_, "fired:" << fired << ",not cancel timer id:" << fired->timer_out_id);
  }

  return fired;
}

void SyncSequence::OnTimer(void* function_data) {
  //TRACE(logger_, "SyncSequence::OnTimer.");
  //超时先关闭连接
  //if (0 == rand() % 5) {
    INFO(logger_, "SyncSequence::OnTimer,fd:"<< sync_client_con_->GetFd()
      <<",sync_client_con_:"<< sync_client_con_);
  //}
  /*EventMessage* message = (EventMessage*)function_data;
  if (message != nullptr && message->msg_coro != nullptr) {
     CoroutineServiceHandler* hdl = (CoroutineServiceHandler*)message->handle_svr;
      //CoroutineServiceHandler* hdl =  CoroutineManager::Instance().GetServiceHandler() 不能这样获取，因为在不同的线程
      if (hdl != nullptr) {
        hdl->AddTimeOutFromClient(message->msg_coro);
      }
  }*/

  sync_client_con_->RegisterDel(sync_client_con_->GetFd());
  sync_client_con_->CleanSequenceQueue();
  sync_client_con_->CancelTimer();
  sync_client_con_->CleanSyncClient();
  sync_client_con_->StartReconnectTimer();
}

std::list<EventMessage*> SyncSequence::Clear() {
  std::list<EventMessage*> tmp;
  lock_.lock();
  tmp.swap(list_);
  lock_.unlock();
  return std::move(tmp);
}

void SyncSequence::CancelTimer(uint64_t timer_id){
  int thread_id = sync_client_con_->GetRegisterThreadId();
  if(timer_id > 0 && thread_id >= 0){
    ServiceManager::GetInstance().GetNetThreadManager()->CancelTimer(timer_id,thread_id);
  } 
}

}

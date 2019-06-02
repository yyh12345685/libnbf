#include "net/sync_sequence.h"
#include "message.h"
#include "common/time.h"
#include "net/sync_client_connect.h"
#include "net/io_handle.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

static const uint32_t check_timer_type = 0;
static const uint32_t real_timer_type = 1;

SyncSequence::SyncSequence(
  SyncClientConnect* sync_client_con,uint32_t timeout_in_ms):
  sync_client_con_(sync_client_con),
  timeout_ms_(timeout_in_ms),
  fired_(nullptr),
  time_check_started_(false){
}

SyncSequence::~SyncSequence(){
}

void SyncSequence::StartTimeCheck(){
  TimerData td;
  td.time_out_ms = 3;
  td.time_proc = this;
  td.function_data = (void*)(&check_timer_type);
  //这里只是用来驱动超时检测,3ms之后执行OnTimer
  IoHandler::GetIoHandler()->StartTimer(td);
}

int SyncSequence::Put(EventMessage* message) {
  message->birthtime = Time::GetMillisecond();
  list_.emplace_back(message);

  if (!time_check_started_){
    StartTimeCheck();
    time_check_started_ = true;
  }
  
  TimerData real_td;
  real_td.time_out_ms = timeout_ms_;
  real_td.time_proc = this;
  real_td.function_data = (void*)(&real_timer_type);
  //这里才是真正的设置超时的地方
  message->timer_out_id = timer_.AddTimer(timeout_ms_,real_td);
  TRACE(logger_, "start timer id:" << message->timer_out_id);
  return 0;
}

EventMessage* SyncSequence::Get() {
  if (nullptr == fired_) {
    return nullptr;
  } else {
    EventMessage* fired = fired_;
    list_.pop_front();
    fired_ = nullptr;

    if (nullptr != fired && fired->timer_out_id > 0) {
      timer_.DelTimer(fired->timer_out_id);
      TRACE(logger_, "have response cancel timer id:" << fired->timer_out_id);
    }

    return fired;
  };
}

EventMessage* SyncSequence::Fired(){
  return fired_;
}

EventMessage* SyncSequence::Fire(){
  TRACE(logger_, "fired_:" << fired_ << ",sync msg size:" << list_.size());
  if (fired_) {
    return nullptr;
  } else {
    return fired_ = list_.front();
  }
}

void SyncSequence::OnTimer(void* function_data){
  uint32_t timer_type = *((uint32_t*)(function_data));
  //TRACE(logger_, "timer_type:" << timer_type);
  switch (timer_type)
  {
  case check_timer_type:
    timer_.ProcessTimer();
    StartTimeCheck();
    break;
  case real_timer_type:
    //超时先关闭连接
    INFO(logger_, "SyncSequence::OnTimer,fd:"<< sync_client_con_->GetFd());
    //sync_client_con_->OnTimeout(Fired());
    sync_client_con_->RegisterDel(sync_client_con_->GetFd());
    sync_client_con_->CleanSequenceQueue();
    sync_client_con_->CancelTimer();
    sync_client_con_->CleanSyncClient();
    sync_client_con_->StartReconnectTimer();
    break;
  default:
    break;
  }
  
}

std::list<EventMessage*> SyncSequence::Clear(){
  std::list<EventMessage*> tmp(list_);
  list_.clear();
  fired_ = nullptr;
  return tmp;
}

}
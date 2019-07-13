#include "net/sync_sequence.h"
#include "message.h"
#include "common/time.h"
#include "net/sync_client_connect.h"
#include "net/io_handle.h"
#include "common/string_util.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

static const uint32_t check_timer_type = 0;
static const uint32_t real_timer_type = 1;

SyncSequence::SyncSequence(
  SyncClientConnect* sync_client_con,uint32_t timeout_in_ms):
  sync_client_con_(sync_client_con),
  timeout_ms_(timeout_in_ms),
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
  lock_.lock();
  list_.emplace_back(message);
  lock_.unlock();

  if (!time_check_started_){
    StartTimeCheck();
    time_check_started_ = true;
  }
  
  TimerData real_td;
  real_td.time_out_ms = timeout_ms_;
  real_td.time_proc = this;
  real_td.function_data = (void*)(&real_timer_type);
  //这里才是真正的设置超时的地方
  time_lock_.lock();
  message->timer_out_id = timer_.AddTimer(timeout_ms_,real_td);
  time_lock_.unlock();
  TRACE(logger_, "start timer id:" << message->timer_out_id);
  return 0;
}

EventMessage* SyncSequence::Get() {
  lock_.lock();
  if (0 == list_.size()) {
    lock_.unlock();
    return nullptr;
  }
  EventMessage* fired = list_.front();
  list_.pop_front();
  lock_.unlock();

  if (nullptr != fired && fired->timer_out_id > 0) {
    time_lock_.lock();
    timer_.DelTimer(fired->timer_out_id);
    time_lock_.unlock();
    TRACE(logger_, "have response cancel timer id:" << fired->timer_out_id);
  }else{
    WARN(logger_, "fired:" << fired << ",not cancel timer id:" << fired->timer_out_id);
  }

  return fired;
}

void SyncSequence::OnTimer(void* function_data){
  uint32_t timer_type = *((uint32_t*)(function_data));
  //TRACE(logger_, "timer_type:" << timer_type);
  switch (timer_type)
  {
  case check_timer_type:{
    std::list<size_t> time_ids;
    time_lock_.lock();
    timer_.ProcessTimerTest(time_ids);
    time_lock_.unlock();
    std::string ids_str;
    for (const auto& id : time_ids) {
      ids_str.append(common::ToString(id)).append("-");
    }
    if (!ids_str.empty()){
      TRACE(logger_, "ProcessTimerTest,ids:" << ids_str);
    }
    StartTimeCheck();
    break;
  }
  case real_timer_type:
    //超时先关闭连接
    if (1 == rand() % 100)
      INFO(logger_, "SyncSequence::OnTimer,fd:"<< sync_client_con_->GetFd()
        <<",sync_client_con_:"<< sync_client_con_);
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

void SyncSequence::ClearTimer() {
  timer_.Clear();
}

std::list<EventMessage*> SyncSequence::Clear(){
  std::list<EventMessage*> tmp;
  lock_.lock();
  tmp.swap(list_);
  lock_.unlock();
  return std::move(tmp);
}

}
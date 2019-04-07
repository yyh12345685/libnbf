#include "net/sync_sequence.h"
#include "message.h"
#include "common/time.h"
#include "net/sync_client_connect.h"
#include "net/io_handle.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

SyncSequence::SyncSequence(
  SyncClientConnect* sync_client_con,uint32_t timeout_in_ms):
  sync_client_con_(sync_client_con),
  timeout_ms_(timeout_in_ms),
  fired_(nullptr){
}

SyncSequence::~SyncSequence(){
}

int SyncSequence::Put(EventMessage* message) {
  message->birthtime = Time::GetMillisecond();
  list_.emplace_back(message);

  TimerData td;
  td.time_out_ms = timeout_ms_;
  td.time_proc = this;
  td.function_data = nullptr;
  IoHandler::GetIoHandler()->StartTimer(td);
  return 0;
}

EventMessage* SyncSequence::Get() {
  if (!fired_) {
    return nullptr;
  } else {
    EventMessage* fired = fired_;
    list_.pop_front();
    fired_ = nullptr;
    return fired;
  };
}

EventMessage* SyncSequence::Fired(){
  return fired_;
}

EventMessage* SyncSequence::Fire(){
  if (fired_) {
    return nullptr;
  } else {
    return fired_ = list_.front();
  }
}

void SyncSequence::OnTimer(void* function_data){
  //超时先关闭连接
  INFO(logger_, "SyncSequence::OnTimer.");
  //sync_client_con_->OnTimeout(Fired());
  sync_client_con_->CleanSequenceQueue();
  sync_client_con_->CancelTimer();
  sync_client_con_->CleanSyncClient();
  sync_client_con_->RegisterDel(sync_client_con_->GetFd());
  sync_client_con_->StartReconnectTimer();
}

std::list<EventMessage*> SyncSequence::Clear(){
  std::list<EventMessage*> tmp(list_);
  list_.clear();
  fired_ = nullptr;
  return tmp;
}

}
#include "net/async_sequence.h"
#include "message.h"
#include "net/io_handle.h"
#include "net/async_client_connect.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncSequence);

AsyncSequence::AsyncSequence(
  AsyncClientConnect* async_client_con, uint32_t timeout_in_ms):
  async_client_con_(async_client_con),
  timeout_ms_(timeout_in_ms){
}

AsyncSequence::~AsyncSequence(){
}

int AsyncSequence::Put(EventMessage* message){
  registery_lock_.lock();
  if (!registery_.insert(std::make_pair(message->sequence_id, message)).second) {
    WARN(logger, "AsyncSequence::Put duplicate sequence_id:"
      << message->sequence_id);
    registery_lock_.unlock();
    return -1;
  }
  registery_lock_.unlock();

  message->timer_out_id = StartTimer(&(message->sequence_id));

  list_lock_.lock();
  list_.emplace_back(message);
  list_lock_.unlock();

  return 0;
}

EventMessage* AsyncSequence::Get(uint64_t sequence_id){
  registery_lock_.lock();
  auto it = registery_.find(sequence_id);
  if (it == registery_.end()) {
    registery_lock_.unlock();
    return nullptr;
  }

  EventMessage* message = it->second;
  registery_.erase(it);
  registery_lock_.unlock();

  IoHandler::GetIoHandler()->CancelTimer(message->timer_out_id);

  list_lock_.lock();
  list_.remove(message);
  list_lock_.unlock();

  return message;
}

void AsyncSequence::OnTimer(void* function_data){
  uint64_t sequence_id = *((uint64_t*)(function_data));
  EventMessage* msg = Get(sequence_id);
  if (nullptr != msg){
    async_client_con_->OnTimeout(msg);
  }
}

std::list<EventMessage*> AsyncSequence::Clear(){
  std::list<EventMessage*> tmp;
  list_lock_.lock();
  tmp.swap(list_);
  list_lock_.unlock();

  registery_lock_.lock();
  registery_.clear();
  registery_lock_.unlock();
  return tmp;
}

uint64_t AsyncSequence::StartTimer(void* data){
  TimerData td;
  td.time_out_ms = timeout_ms_;
  td.time_proc = this;
  td.function_data = data ;

  return IoHandler::GetIoHandler()->StartTimer(td);
}

void AsyncSequence::CancelTimer(uint64_t timer_id){
  IoHandler::GetIoHandler()->CancelTimer(timer_id);
}

}

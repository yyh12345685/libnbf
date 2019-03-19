#include "net/async_sequence.h"
#include "common/time.h"
#include "message.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncSequence);

AsyncSequence::AsyncSequence(uint32_t timeout_in_ms):
  timeout_ms_(timeout_in_ms){
}

AsyncSequence::~AsyncSequence(){
}

int AsyncSequence::Put(EventMessage* message){
  message->birthtime = Time::GetMillisecond();

  if (!registery_.insert(std::make_pair(message->sequence_id, message)).second) {
    WARN(logger, "AsyncSequence::Put duplicate sequence_id:"
      << message->sequence_id);
    return -1;
  }

  list_.emplace_back(message);
  return 0;
  return 0;
}

EventMessage* AsyncSequence::Get(uint32_t sequence_id){
  auto it = registery_.find(sequence_id);
  if (it == registery_.end()) {
    return NULL;
  }

  EventMessage* message = it->second;
  registery_.erase(it);

  list_.remove(message);
  return message;
}

std::list<EventMessage*> AsyncSequence::Timeout(){
  uint64_t current_timestamp = Time::GetMillisecond();
  std::list<EventMessage*> tmp;
  EventMessage* message = list_.front();
  while (message && (message->birthtime+timeout_ms_) < current_timestamp){
    list_.pop_front();
    tmp.push_back(message);
    registery_.erase(message->sequence_id);
    message = list_.front();
  }
  return tmp;
}

std::list<EventMessage*> AsyncSequence::Clear(){
  std::list<EventMessage*> tmp;
  tmp.swap(list_);
  registery_.clear();
  return tmp;
}

}

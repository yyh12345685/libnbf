#include "net/sync_sequence.h"
#include "message.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

SyncSequence::SyncSequence(uint32_t timeout_in_ms):
  timeout_ms_(timeout_in_ms),
  fired_(NULL){
}

SyncSequence::~SyncSequence(){
}

int SyncSequence::Put(EventMessage* message) {
  message->birthtime = Time::GetMillisecond();
  list_.emplace_back(message);
  return 0;
}

EventMessage* SyncSequence::Get() {
  if (!fired_) {
    return NULL;
  } else {
    EventMessage* fired = fired_;
    list_.pop_front();
    fired_ = NULL;
    return fired;
  };
}

EventMessage* SyncSequence::Fired(){
  return fired_;
}

EventMessage* SyncSequence::Fire(){
  if (fired_) {
    return NULL;
  } else {
    return fired_ = list_.front();
  }
}

std::list<EventMessage*> SyncSequence::Timeout() {
  if (!fired_) {
    return std::list<EventMessage*>();
  }

  uint64_t current_timestamp = Time::GetMillisecond();
  if ((fired_->birthtime+timeout_ms_) < current_timestamp) {
    return Clear();
  } else {
    return std::list<EventMessage*>();
  }
}

std::list<EventMessage*> SyncSequence::Clear(){
  std::list<EventMessage*> tmp(list_);
  list_.clear();
  fired_ = NULL;
  return tmp;
}

}
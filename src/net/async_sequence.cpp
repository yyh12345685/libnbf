#include "net/async_sequence.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncSequence);

int AsyncSequence::Put(EventMessage* message){
  return 0;
}

EventMessage* AsyncSequence::Get(uint32_t sequence_id){
  return NULL;
}

std::queue<EventMessage*> AsyncSequence::Timeout(){
  return std::queue<EventMessage*>();
}

}

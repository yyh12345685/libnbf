#include "net/async_sequence.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncSequence);

int AsyncSequence::Put(MessageBase* message){
  return 0;
}

MessageBase* AsyncSequence::Get(uint32_t sequence_id){
  return NULL;
}

std::queue<MessageBase*> AsyncSequence::Timeout(){

}

}

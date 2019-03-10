#include "net/sync_sequence.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, SyncSequence);

int SyncSequence::Put(MessageBase* message) {
  return 0;
}

MessageBase* SyncSequence::Get(uint32_t sequence_id) {
  return NULL;
}

std::queue<MessageBase*> SyncSequence::Timeout() {

}

}
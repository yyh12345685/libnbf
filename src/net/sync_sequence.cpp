#include "net/sync_sequence.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, SyncSequence);

int SyncSequence::Put(EventMessage* message) {
  return 0;
}

EventMessage* SyncSequence::Get(uint32_t sequence_id) {
  return NULL;
}

std::queue<EventMessage*> SyncSequence::Timeout() {
  return std::queue<EventMessage*>();
}

}
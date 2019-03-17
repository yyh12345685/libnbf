
#include "net/sync_client_connect.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, SyncClientConnect);

SyncClientConnect::SyncClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  ClientConnect(timeout_ms,heartbeat_ms){
}

SyncClientConnect::~SyncClientConnect(){
}

void SyncClientConnect::OnDecodeMessage(EventMessage* message) {

}

}



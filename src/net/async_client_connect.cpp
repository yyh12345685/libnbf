#include "net/async_client_connect.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncClientConnect);

AsyncClientConnect::AsyncClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  ClientConnect(timeout_ms,heartbeat_ms){
}

AsyncClientConnect::~AsyncClientConnect(){
}

void AsyncClientConnect::OnDecodeMessage(EventMessage* message){

}

}

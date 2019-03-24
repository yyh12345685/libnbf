
#include "message_base.h"
#include "protocol/protocol_base.h"
#include "protocol/http/http_protocol.h"
#include "protocol/rapid/rapid_protocol.h"
#include "protocol/redis/redis_protocol.h"

namespace bdf{

ProtocolFactory::ProtocolFactory() {
  RegisterProtocol(MessageType::kRapidMessage, BDF_NEW(RapidProtocol));
  RegisterProtocol(MessageType::kHttpMessage, BDF_NEW(HttpProtocol));
  RegisterProtocol(MessageType::kRedisMessage, BDF_NEW(RedisProtocol));
}

ProtocolFactory::~ProtocolFactory() {
  for (auto protocol : protocol_) {
    if (protocol) 
      BDF_DELETE(protocol);
  }
}

ProtocolBase* ProtocolFactory::Create(int type) {
  if (type <= 0 || type >= (int)protocol_.size()) {
    return nullptr;
  }

  ProtocolBase* protocol = protocol_.at(type);
  if (!protocol) {
    return nullptr;
  }

  return protocol->Clone();
}

void ProtocolFactory::RegisterProtocol(int type, ProtocolBase* protocol) {
  if (type >= (int)protocol_.size()) {
    protocol_.resize(type + 1);
  }

  protocol_[type] = protocol;
}

}


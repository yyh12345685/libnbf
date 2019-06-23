#pragma once

#include <string>
#include <atomic>
#include <vector>
#include "common/logger.h"

namespace bdf {

class EventMessage;
class ClientRouter;
class ClientConfig;

class ClientRouters {
public:
  ClientRouters(const std::string& name, const std::string& mapping);
  ~ClientRouters();

  int Start(const std::vector<ClientConfig>& clients);
  int Stop();

  bool Send(EventMessage * message);
  bool SendHash(EventMessage* message, uint32_t hash);

  EventMessage* SendRecieve(EventMessage* message, uint32_t timeout_ms = 0);
  EventMessage* SendRecieveHash(
    EventMessage* message, uint32_t hash, uint32_t timeout_ms = 0);

private:
  LOGGER_CLASS_DECL(logger_);

  std::string name_;
  //可以根据发送的msg的某个值hash到某个服务器和ip上，暂未实现
  std::string mapping_;
  std::vector<ClientRouter*> client_routers_;
  std::atomic<uint32_t> current_;
};

}


#pragma once

#include <map>
#include <string>
#include "common/logger.h"
#include "common/common.h"

namespace bdf{

struct ClientRoutersConfig;
class ClientRouters;
class EventMessage;

class ClientMgr{

public:

  ClientMgr();
  ~ClientMgr();

  int Start(const ClientRoutersConfig& routers_config);
  int Stop();

  bool Send(const std::string& router,EventMessage* message);
  bool SendHash(const std::string& router, EventMessage* message, uint32_t hash);

  EventMessage* SendRecieve(
    const std::string& router,
    EventMessage* message,
    uint32_t timeout_ms = 0);
  EventMessage* SendRecieveHash(
    const std::string& router,
    EventMessage* message,
    uint32_t hash,
    uint32_t timeout_ms = 0);

private:
  LOGGER_CLASS_DECL(logger_);
  std::map<std::string, ClientRouters* > router_maps_;

  ClientRouters* GetClientRouters(const std::string& router);

  DISALLOW_COPY_AND_ASSIGN(ClientMgr)

};

}


#pragma once

#include <string>
#include <atomic>
#include <vector>
#include "common/logger.h"

namespace bdf{

class EventMessage;
class Client;
struct ClientConfig;

class ClientRouter{
public:
  ClientRouter(const std::string& name, const bool& sigle_send_sigle_recv);
  ~ClientRouter();

  int Start(const ClientConfig&cli_config);
  int Stop();

  bool Send(EventMessage * message);

  EventMessage* SendRecieve(EventMessage* message, uint32_t timeout_ms = 0){
    return DoSendRecieve(message, timeout_ms);
  }

protected:
  Client* GetValidClient();

private:
  LOGGER_CLASS_DECL(logger_);

  EventMessage* DoSendRecieve(EventMessage* message, uint32_t timeout_ms = 0);

  std::string name_;
  std::vector<Client*> clients_;
  std::atomic<uint32_t> current_;

  bool sigle_send_sigle_recv_;
};

}

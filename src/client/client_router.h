#pragma once

#include <string>
#include <atomic>
#include <vector>
#include "common/logger.h"
#include "context.h"

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

  bool Invoke(EventMessage * message, const InvokerCallback& cb);

  EventMessage* SendRecieve(EventMessage* message){
    return DoSendRecieve(message);
  }

protected:
  Client* GetValidClient();

private:
  LOGGER_CLASS_DECL(logger_);

  EventMessage* DoSendRecieve(EventMessage* message);

  std::string name_;
  std::vector<Client*> clients_;
  std::atomic<uint32_t> current_;

  bool sigle_send_sigle_recv_;
};

}

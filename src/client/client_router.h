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
  // Next connected client with spare capacity (round robin). Saturated
  // clients are skipped so one slow connection does not fail its share of
  // requests while the others are idle. for_invoke: also require room under
  // the in-flight limit (which only Invoke uses).
  Client* GetValidClient(bool for_invoke);

private:
  LOGGER_CLASS_DECL(logger_);

  EventMessage* DoSendRecieve(EventMessage* message);

  std::string name_;
  std::vector<Client*> clients_;
  std::atomic<uint32_t> current_;

  bool sigle_send_sigle_recv_;
};

}

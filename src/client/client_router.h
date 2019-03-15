#pragma once

#include <string>
#include <atomic>
#include <vector>
#include "common/logger.h"

namespace bdf{

class EventMessage;
class Client;

class ClientRouter{
public:
  ClientRouter(const std::string& name);
  ~ClientRouter();

  int AddClient(const std::string& address, uint32_t timeout_ms);

  int Start();
  int Stop();

  bool Send(EventMessage * message);

  template<typename T>
  typename T::ResponseType * SendRecieve(T* message, uint32_t timeout_ms = 0) {
    return (typename T::ResponseType*)DoSendRecieve(message, timeout_ms);
  }

private:
  LOGGER_CLASS_DECL(logger);

  EventMessage* DoSendRecieve(EventMessage* message, uint32_t timeout_ms = 0);

  Client* GetNextClient();
  Client* GetHashClient(uint32_t hash);

  std::string name_;
  std::vector<Client*> clients_;
  std::atomic<uint32_t> current_;
};

}

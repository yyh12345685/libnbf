#pragma once

#include <string>
#include "common/logger.h"
#include "net/client_connect.h"

namespace bdf{

class EventMessage;

class Client {
public:
  enum {
    kWorking = 0,
    kBroken = 1,
  };

  Client(
    const std::string& name,
    const std::string& address,
    uint32_t timeout_ms,
    uint32_t heartbeat_ms);

  ~Client();

  inline const std::string& GetName() const { return name_; }
  const std::string& GetAddress() const { return address_; }
  uint32_t GetTimeout() const { return timeout_ms_; }
  uint32_t GetHeartBeat() const { return heartbeat_ms_; }


  int Start();
  int Stop();

  bool Send(EventMessage* message);
  EventMessage* SendRecieve(EventMessage* message, uint32_t timeout_ms = 0){
    return DoSendRecieve(message, timeout_ms);
  }

  int GetClientStatus() const {
    return connect_ && connect_->GetStatus() == 
      ClientConnect::kConnected ? kWorking : kBroken;
  }

private:
  LOGGER_CLASS_DECL(logger);

  ClientConnect* CreateClient(
    const std::string& address, uint32_t timeout_ms, uint32_t heartbeat_ms);

  EventMessage* DoSendRecieve(EventMessage* message, uint32_t timeout_ms = 0);

  static int64_t GetSequenceId();

  std::string name_;
  std::string address_;
  uint32_t timeout_ms_;
  uint32_t heartbeat_ms_;

  ClientConnect* connect_;
};

}

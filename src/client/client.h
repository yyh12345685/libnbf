#pragma once

#include <string>
#include <atomic>
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
    const std::string& address,
    uint32_t timeout_ms,
    uint32_t heartbeat_ms,
    const bool& sigle_send_sigle_recv);

  ~Client();

  const std::string& GetAddress() const { return address_; }
  uint32_t GetTimeout() const { return timeout_ms_; }
  uint32_t GetHeartBeat() const { return heartbeat_ms_; }


  int Start();
  int Stop();

  bool Send(EventMessage* message);

  bool Invoke(EventMessage* message, const InvokerCallback& cb,const std::string& name);

  EventMessage* SendRecieve(EventMessage* message){
    return DoSendRecieve(message);
  }

  // connect_ is cleared by Stop() while other threads may be in here:
  // load it once and null-check (it used to be read twice / dereferenced).
  int GetClientStatus() const {
    ClientConnect* con = connect_.load();
    return con && con->GetStatus() ==
      ClientConnect::kConnected ? kWorking : kBroken;
  }

  void SetNoBuzy(){
    ClientConnect* con = connect_.load();
    if (!sigle_send_sigle_recv_ || !con){
      return ;
    }
    return con->SetBuzy(false);
  }

  bool TrySetBusy() {
    if (!sigle_send_sigle_recv_) {
      return  true;
    }
    ClientConnect* con = connect_.load();
    return con && con->TrySetBuzy();
  }

  void Dump(std::ostream& os) const;

private:
  LOGGER_CLASS_DECL(logger_);

  ClientConnect* CreateClient(const std::string& address,uint32_t heartbeat_ms);

  EventMessage* DoSendRecieve(EventMessage* message);
  void DoSend(EventMessage* message);
  EventMessage* DoRecieve(uint64_t sequence_id);

  static int64_t GetSequenceId();

  std::string address_;
  uint32_t timeout_ms_;
  uint32_t heartbeat_ms_;

  std::atomic<ClientConnect*> connect_;

  bool sigle_send_sigle_recv_;
};

std::ostream& operator << (std::ostream& os, const Client& client);

}

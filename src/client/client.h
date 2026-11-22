#pragma once

#include <string>
#include <mutex>
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

  int GetClientStatus() const {
    std::lock_guard<std::mutex> guard(connect_lock_);
    return connect_ && connect_->GetStatus() ==
      ClientConnect::kConnected ? kWorking : kBroken;
  }

  void SetNoBuzy(){
    std::lock_guard<std::mutex> guard(connect_lock_);
    if (!sigle_send_sigle_recv_ || !connect_){
      return ;
    }
    connect_->SetBuzy(false);
  }

  bool TrySetBusy() {
    if (!sigle_send_sigle_recv_) {
      return  true;
    }
    std::lock_guard<std::mutex> guard(connect_lock_);
    return connect_ && connect_->GetStatus() == ClientConnect::kConnected
      && connect_->TrySetBuzy();
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

  mutable std::mutex connect_lock_;
  ClientConnect* connect_;

  bool sigle_send_sigle_recv_;
};

std::ostream& operator << (std::ostream& os, const Client& client);

}

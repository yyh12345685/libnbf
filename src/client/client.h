#pragma once

#include <string>
#include <mutex>
#include <memory>
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

  // Connected and below its limits, so a request sent now will not be
  // rejected for backpressure (checked again on send; this is a hint for the
  // router to skip saturated connections). for_invoke also checks the
  // in-flight limit, which applies to Invoke only.
  bool HasCapacity(bool for_invoke) const;

  void Dump(std::ostream& os) const;

private:
  // Effective client_max_inflight. Synchronous (pipelined, in-order)
  // protocols such as HTTP are capped at kMaxSyncInflight: the libnbf server
  // closes an HTTP connection with more than 1024 unanswered requests
  // (kMaxPendingHttpRequests), failing everything in flight on it.
  static const uint32_t kMaxSyncInflight = 1000;
  uint32_t InflightLimit() const;
  // Quiet checks, caller holds connect_lock_ and connect_ is non-null.
  bool InflightAtLimit() const;
  bool QueueAtLimit(size_t* pending) const;
  // True when the I/O thread owning connect_ has client_send_queue_limit
  // messages queued; caller holds connect_lock_ and connect_ is non-null.
  bool SendQueueFull();
  // True when this client already has client_max_inflight Invokes whose
  // response message is not yet destroyed.
  bool InflightFull();
  // Rate-limited "channel broken" log (once per second across all clients).
  void LogBroken(const char* where);
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
  // Invokes from this client not yet completed (see ContextBase::inflight).
  std::shared_ptr<std::atomic<int64_t> > inflight_;
  // Protocol of connect_ is synchronous (set in Start).
  bool synchronous_ = false;

  bool sigle_send_sigle_recv_;
};

std::ostream& operator << (std::ostream& os, const Client& client);

}

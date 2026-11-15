#pragma once

#include <atomic>
#include <vector>
#include "net/socket.h"
#include "net/connect.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"

namespace bdf{

//class  HeartBeatTimer:public OnTimerBase{
//public:
//  virtual void OnTimer(void* function_data);
//};

class ClientConnect:public Connecting,public OnTimerBase {
public:
  //friend class HeartBeatTimer;
  enum {
    kConnecting = 0,
    kConnected,
    kBroken,
    kStopping,
  };

  ClientConnect(uint32_t timeout_ms, uint32_t heartbeat_ms);
  virtual ~ClientConnect();

  virtual void OnTimer(void* function_data);

  int TryConnect();
  int Stop();
  // Runs on the owning I/O thread (kIoActiveCloseEvent).
  virtual void Destroy();

  int RegisterAddModrw(int fd,int& register_thread_id);
  int RegisterAddModr(int fd,int& register_thread_id);
  int RegisterDel(int fd);

  int GetRegisterThreadId(){
    return register_thread_id_;
  }

  inline int GetStatus() const { return status_; }
  inline void SetStatus(int status) { status_ = status; }

  virtual void OnClose();

  // All timers of a connection live on its own I/O loop (the same thread
  // that reads, writes and closes it), so their callbacks never race with
  // the connection. They used to run on a separate ClientReconnect thread.
  // delay_ms 0: the reconnect interval.
  int StartReconnectTimer(uint32_t delay_ms = 0);
  int StartHeartBeatTimer();
  void CancelTimer();

  virtual void SetBuzy(bool is_buzy) = 0;
  virtual bool TrySetBuzy() = 0;

protected:
  enum {
    kUnKown = 0,
    kClientTimerTypeReconnect = 1,
    kClientTimerTypeHeartBeat = 2,
  };
  uint32_t client_timer_type_reconnect_ = kClientTimerTypeReconnect;
  uint32_t client_timer_type_heartbeat_ = kClientTimerTypeHeartBeat;

  void OnWrite();
  void OnConnectWrite();
  bool IsConnected(uint32_t time_ms, int wait_time = 1);

  virtual void OnHeartBeat();
  virtual void CleanSequenceQueue() = 0;

  void DoSendBack(EventMessage* message, int status);
  void CleanClient();

  bool is_buzy_;
private:
  LOGGER_CLASS_DECL(logger);

  std::atomic<int> status_; // read by service threads (GetClientStatus)

  uint32_t timeout_ms_;
  uint32_t heartbeat_ms_;

  uint64_t reconnect_timer_;
  uint64_t heartbeat_timer_;

  int register_thread_id_;

  int TimerThreadId();

  // Resolved once, on the first TryConnect (caller's thread). Reconnects run
  // on the I/O loop and must not block on DNS, so they use this cache and
  // move to the next address after each failed attempt.
  std::vector<Socket::ResolvedAddr> addrs_;
  size_t addr_index_ = 0;
  void NextAddr() { if (!addrs_.empty()) addr_index_ = (addr_index_ + 1) % addrs_.size(); }

  enum : uint32_t { kReconnectIntervalMs = 300, kConnectTimeoutMs = 3000 };

  //HeartBeatTimer heart_beat_timer_;
};


}
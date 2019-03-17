#pragma once

#include "net/connect.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"

namespace bdf{

class ClientConnect:public Connecting,public OnTimerBase {
public:

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

  virtual int RegisterAddModrw(int fd);
  virtual int RegisterDel(int fd);

  inline int GetStatus() const { return status_; }
  inline void SetStatus(int status) { status_ = status; }

  virtual void OnClose();

protected:
  enum {
    kUnKown = 0,
    kClientTimerTypeReconnect = 1,
    kClientTimerTypeTimeout,
    kClientTimerTypeHeartBeat£¬
  };
  uint8_t client_timer_type_reconnect_ = kClientTimerTypeReconnect;
  uint8_t client_timer_type_timeout_ = kClientTimerTypeTimeout;
  uint8_t client_timer_type_heartbeat_ = kClientTimerTypeHeartBeat£¬;
  int StartReconnectTimer();
  int StartTimeoutTimer();
  int StartHeartBeatTimer();
  void CancelTimer();
private:
  LOGGER_CLASS_DECL(logger);

  int status_;

  uint32_t timeout_ms_;
  uint32_t heartbeat_ms_;

  uint64_t reconnect_timer_;
  uint64_t timeout_timer_;
  uint64_t heartbeat_timer_;
};


}
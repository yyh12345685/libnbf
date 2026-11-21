#pragma once

#include "net/connect.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;

class ServerConnect :public Connecting, public OnTimerBase {

public:
  ServerConnect();
  virtual ~ServerConnect();

  virtual void Destroy();

  virtual void OnClose();
  virtual void OnDecodeMessage(EventMessage* message);
  virtual int RegisterDel(int fd);

  virtual bool IsServer() { return true; }

  virtual void OnWrite();
  virtual void OnReplyEncoded() { if (in_flight_ > 0) --in_flight_; }
  // Linger timeout after a half-close.
  virtual void OnTimer(void* function_data);

protected:
  // Half-close (client sent its request, then shutdown(SHUT_WR)): keep the
  // connection until the outstanding replies are written, instead of
  // resetting it and dropping them.
  virtual void OnPeerClosed();

private:
  LOGGER_CLASS_DECL(logger_);

  bool released_ = false; // only touched on the owning I/O thread
  // All below: owning I/O thread only.
  int in_flight_ = 0;      // requests handed to handlers, not yet answered
  bool peer_closed_ = false;
  uint64_t linger_timer_ = 0;
  void CancelLinger();
  void MaybeFinishHalfClose();
  enum { kHalfCloseLingerMs = 5000 };
};

}

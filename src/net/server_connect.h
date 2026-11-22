#pragma once

#include <map>

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
  void SendReply(EventMessage* message);
  // Linger timeout after a half-close.
  virtual void OnTimer(void* function_data);

protected:
  // Half-close (client sent its request, then shutdown(SHUT_WR)): keep the
  // connection until the outstanding replies are written, instead of
  // resetting it and dropping them.
  virtual void OnPeerClosed();

private:
  LOGGER_CLASS_DECL(logger_);

  void ClearPendingReplies();
  // HTTP needs replies in request order; computed once (protocol is fixed
  // after accept) instead of a dynamic_cast per message.
  bool IsOrdered();
  int ordered_state_ = -1; // -1 unknown, 0 no, 1 yes
  uint64_t next_request_id_ = 0;
  uint64_t next_reply_id_ = 0;
  std::map<uint64_t, EventMessage*> pending_replies_;
  bool released_ = false; // only touched on the owning I/O thread
  // All below: owning I/O thread only.
  int in_flight_ = 0;      // requests handed to handlers, not yet answered
  bool peer_closed_ = false;
  uint64_t linger_timer_ = 0;
  // Armed while later replies wait behind a missing earlier one: a handler
  // that never answers must not block the connection forever.
  uint64_t reply_wait_timer_ = 0;
  bool close_after_reply_ = false;
  void StartLingerTimer();
  void CancelLinger();
  void UpdateReplyWaitTimer();
  void CancelReplyWait();
  void MaybeFinishHalfClose();
  enum { kHalfCloseLingerMs = 5000, kReplyWaitMs = 30000 };
};

}

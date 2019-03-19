#pragma once

#include "net/client_connect.h"
#include "net/sync_sequence.h"
#include "common/logger.h"

namespace bdf{

class SyncClientConnect:public ClientConnect{

public:
  SyncClientConnect(uint32_t timeout_ms, uint32_t heartbeat_ms);
  ~SyncClientConnect();

  virtual void OnDecodeMessage(EventMessage* message);
  virtual int EncodeMsg(EventMessage* message);
protected:
  virtual void OnTimeout();
  virtual void CleanSequenceQueue();

  virtual int DecodeMsg();

  void FireMessage();

private:
  SyncSequence sync_sequence_;

  LOGGER_CLASS_DECL(logger);
};

}
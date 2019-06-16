#pragma once

#include "net/client_connect.h"
#include "net/sync_sequence.h"
#include "common/logger.h"

namespace bdf{

class SyncClientConnect:public ClientConnect{

public:
  SyncClientConnect(uint32_t timeout_ms, uint32_t heartbeat_ms);
  ~SyncClientConnect();

  virtual void Destroy() {
    BDF_DELETE(this);
  }

  virtual void OnDecodeMessage(EventMessage* message);
  virtual int EncodeMsg(EventMessage* message);

  void OnTimeout(EventMessage* msg);
  virtual void CleanSequenceQueue();

  void CleanSyncClient();
protected:
  virtual int DecodeMsg();
  
private:
  SyncSequence sync_sequence_;

  //std::mutex test_lock_;
  //std::mutex test_lock1_;

  LOGGER_CLASS_DECL(logger);
};

}
#pragma once

#include "net/client_connect.h"
#include "net/async_sequence.h"
#include "common/logger.h"

namespace bdf {

class AsyncClientConnect :public ClientConnect {

public:
  AsyncClientConnect(uint32_t timeout_ms, uint32_t heartbeat_ms);
  virtual ~AsyncClientConnect();

  virtual void Destroy() {
    BDF_DELETE(this);
  }

  virtual void OnDecodeMessage(EventMessage* message);
  virtual int EncodeMsg(EventMessage* message);

  void OnTimeout(EventMessage* msg);
protected:
  virtual void CleanSequenceQueue();

private:
  AsyncSequence async_sequence_;
  LOGGER_CLASS_DECL(logger);
};

}

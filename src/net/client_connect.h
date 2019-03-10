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

  ClientConnect();
  ~ClientConnect();

  virtual void OnTimer(void* function_data);

  void TryConnect();

private:
  LOGGER_CLASS_DECL(logger);
};


}
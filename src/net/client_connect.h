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

  virtual int RegisterAddModr(int fd, bool set);
  virtual int RegisterModr(int fd, bool set);
  virtual int RegisterModw(int fd, bool set);
  virtual int RegisterDel(int fd);

private:
  LOGGER_CLASS_DECL(logger);
};


}
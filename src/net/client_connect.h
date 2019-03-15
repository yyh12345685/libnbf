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

  int TryConnect();
  int Stop();

  virtual int RegisterAddModrw(int fd, bool set);
  virtual int RegisterDel(int fd);

  inline int GetStatus() const { return status_; }
  inline void SetStatus(int status) { status_ = status; }

private:
  LOGGER_CLASS_DECL(logger);

  int status_;
};


}
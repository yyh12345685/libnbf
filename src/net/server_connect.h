#pragma once

#include "net/connect.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;

class ServerConnect :public Connecting {

public:
  ServerConnect();
  virtual ~ServerConnect();

  virtual void Destroy() {
    BDF_DELETE(this);
  }

  virtual void OnClose();
  virtual void OnDecodeMessage(EventMessage* message);
  virtual int RegisterDel(int fd);

  virtual bool IsServer() { return true; }

private:
  LOGGER_CLASS_DECL(logger);
};

}

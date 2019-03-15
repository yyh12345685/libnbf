#pragma once

#include "net/connect.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;

class ServerConnect :public Connecting {

public:
  ServerConnect();
  ~ServerConnect();

  virtual void OnDecodeMessage(EventMessage* message);

  virtual int RegisterDel(int fd);

private:
  LOGGER_CLASS_DECL(logger);
};

}

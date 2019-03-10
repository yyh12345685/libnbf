#pragma once

#include "net/connect.h"
#include "common/logger.h"

namespace bdf {

class MessageBase;

class ServerConnect :public Connecting {

public:
  ServerConnect();
  ~ServerConnect();

  virtual void OnDecodeMessage(MessageBase* message);

private:
  LOGGER_CLASS_DECL(logger);
};

}

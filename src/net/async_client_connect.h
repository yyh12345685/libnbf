#pragma once

#include "net/client_connect.h"
#include "net/async_sequence.h"
#include "common/logger.h"

namespace bdf {

class AsyncClientConnect :public ClientConnect {

public:
  AsyncClientConnect();
  ~AsyncClientConnect();

  virtual void OnDecodeMessage(EventMessage* message);

private:
  AsyncSequence async_sequence_;
  LOGGER_CLASS_DECL(logger);
};

}

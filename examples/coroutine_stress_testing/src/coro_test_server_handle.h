#pragma once

#include "service/coroutine_service_handle.h"
#include "common/logger.h"

namespace bdf{

namespace testserver{

class CoroTestServerHandler: public CoroutineServiceHandler {
public:
  CoroTestServerHandler() {
  }
  virtual ~CoroTestServerHandler() {
  }

  virtual ServiceHandler* Clone() {
    return new CoroTestServerHandler();
  }

protected:

  virtual void OnRapidRequestMessage(RapidMessage* message);
  virtual void OnHttpRequestMessage(HttpMessage* message);
private:
  LOGGER_CLASS_DECL(logger_);

};

}
}


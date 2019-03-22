#pragma once
#include "service/coroutine_service_handle.h"
#include "common/logger.h"

namespace bdf{

namespace testserver{

class AppTestServerHandler: public CoroutineServiceHandler {
public:
  AppTestServerHandler() {}
  virtual ~AppTestServerHandler() {}

  virtual ServiceHandler* Clone() {
    return new AppTestServerHandler();
  }

  virtual void OnTimer(void* function_data);
protected:
  virtual void OnHttpRequestMessage(HttpMessage* message);
  virtual void OnHttpResponseMessage(HttpMessage* message);

  virtual void OnRapidRequestMessage(RapidMessage* message);
  virtual void OnRapidResponseMessage(RapidMessage* message);

private:
  LOGGER_CLASS_DECL(logger_);

};

}
}


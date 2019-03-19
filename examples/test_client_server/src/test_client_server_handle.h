#pragma once
#include "service/service_handle.h"
#include "common/logger.h"

namespace bdf{

namespace testserver{

class TestClientServerHandler: public ServiceHandler {
public:
  TestClientServerHandler(){}
  virtual ~TestClientServerHandler() {}

  virtual ServiceHandler* Clone() {
    return new TestClientServerHandler();
  }
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


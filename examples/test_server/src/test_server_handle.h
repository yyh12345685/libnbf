#pragma once
#include "service/service_handle.h"
#include "common/logger.h"

namespace bdf{

namespace testserver{

class TestHandler: public ServiceHandler {
public:
  TestHandler(){}
  virtual ~TestHandler() {}

  virtual ServiceHandler* Clone() {
    return new TestHandler();
  }
protected:
  virtual void OnHttpRequestMessage(HttpMessage* message);
  virtual void OnRapidRequestMessage(RapidMessage* message);

private:
  LOGGER_CLASS_DECL(logger_);

};

}
}


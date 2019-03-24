#pragma once

#include "common/logger.h"
#include "handle.h"

namespace bdf{

class EventMessage;
class HttpMessage;
class RapidMessage;
struct HandleData;

class ServiceHandler:public Handler {
public:
  ServiceHandler(){
  }
  virtual ~ServiceHandler(){
  }
  virtual void Run(HandleData* data);

  virtual ServiceHandler* Clone() {
    return new ServiceHandler();
  }

protected:
  void Handle(EventMessage* message);

  virtual void OnHttpRequestMessage(HttpMessage* message);
  virtual void OnHttpResponseMessage(HttpMessage* message);

  virtual void OnRapidRequestMessage(RapidMessage* message);
  virtual void OnRapidResponseMessage(RapidMessage* message);

private:
  LOGGER_CLASS_DECL(logger);

};

}

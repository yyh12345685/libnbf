#pragma once

#include "common/logger.h"

namespace bdf{

class MessageBase;
class HttpMessage;
class RapidMessage;

class ServiceHandler{
public:
  virtual void Run();

protected:

  virtual void Handle(MessageBase* message);

  virtual void OnHttpRequestMessage(HttpMessage* message);
  virtual void OnHttpResponseMessage(HttpMessage* message);

  virtual void OnRapidRequestMessage(RapidMessage* message);
  virtual void OnRapidResponseMessage(RapidMessage* message);

private:
  LOGGER_CLASS_DECL(logger);

};

}

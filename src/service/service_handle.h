#pragma once

#include "common/logger.h"
#include "handle.h"

namespace bdf{

class EventMessage;
class HttpMessage;
class RapidMessage;
struct RedisMessage;
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

  void HttpRequestMessage(HttpMessage* message);
  void HttpResponseMessage(HttpMessage* message);

  void RapidRequestMessage(RapidMessage* message);
  void RapidResponseMessage(RapidMessage* message);
  void RedisResponseMessage(RedisMessage* message);

  virtual void OnHttpRequestMessage(HttpMessage* message);
  virtual void OnHttpResponseMessage(HttpMessage* message);

  virtual void OnRapidRequestMessage(RapidMessage* message);
  virtual void OnRapidResponseMessage(RapidMessage* message);

  // Redis is client-only: replies without an Invoke callback end up here.
  virtual void OnRedisResponseMessage(RedisMessage* message);

private:
  LOGGER_CLASS_DECL(logger_);

};

}

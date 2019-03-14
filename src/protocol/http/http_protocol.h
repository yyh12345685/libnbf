
#pragma once

#include <string.h>
#include <stdlib.h>

#include "protocol/protocol_base.h"
#include "common/logger.h"

namespace bdf {

class EventMessage;
class Buffer;

struct HttpAgent;
class HttpMessage;

class HttpProtocol : public ProtocolBase
{
public:
  HttpProtocol();
  virtual ~HttpProtocol();

  virtual bool IsSynchronous() {
    return true;
  }

  virtual void Release() { 
    delete this; 
  }

  virtual ProtocolBase* Clone() {
    return new HttpProtocol(); 
  }

  virtual EventMessage *Decode(Buffer &input, bool& failed);
  virtual bool Encode(EventMessage *msg, Buffer *output);
private:
  bool WirteToBuf(HttpMessage *msg, Buffer *output);
  LOGGER_CLASS_DECL(logger_);
  HttpAgent* agent_;
};

}


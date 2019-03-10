
#pragma once
#include <vector>
#include "message_base.h"

namespace bdf {

class MessageBase;
class Buffer;

class ProtocolBase {
public:
  enum {
    kDecodeSuccess = 0,
    kDecodeContinue = 1,
    kDecodeFail = 2,
  };
  enum {
    kEncodeSuccess = 0,
    kEncodeFail = 1,
  };
public:
  virtual bool IsSynchronous() = 0;
  virtual void Release() = 0;
  virtual MessageBase *Decode(Buffer &input, bool &failed) = 0;
  virtual bool Encode(MessageBase *msg, Buffer *output) = 0;
  virtual ProtocolBase* Clone() = 0;
protected:
  virtual ~ProtocolBase() {}
};

class ProtocolFactory {
public:
  ProtocolFactory();

  ~ProtocolFactory();

  ProtocolBase* Create(int type);

private:
  LOGGER_CLASS_DECL(logger);

  void RegisterProtocol(int type, ProtocolBase* protocol);

  std::vector<ProtocolBase*> protocol_;
};

}

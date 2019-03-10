#pragma once

#include "event/event_data.h"
#include "common/buffer.h"
#include "common/logger.h"

namespace bdf{

class EventDriver;
class MessageBase;

class Connecting : public EventFunctionBase{
public:

  Connecting();
  ~Connecting();

  virtual void OnEvent(EventDriver *poll, int fd, short event);

protected:

  virtual void OnDecodeMessage(MessageBase* message) = 0;

  virtual void OnRead();
  virtual void OnWrite();

  virtual void OnCloseConnection(int fd);
  virtual void OnError();

private:
  int fd_;
  Buffer inbuf_;
  Buffer outbuf_;

  LOGGER_CLASS_DECL(logger);
};

}


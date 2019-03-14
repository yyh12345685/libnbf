#pragma once

#include "event/event_data.h"
#include "common/buffer.h"
#include "common/logger.h"
#include "protocol/protocol_base.h"

namespace bdf{

class EventDriver;
class EventMessage;
class IoService;

class Connecting : public EventFunctionBase{
public:

  Connecting();
  virtual ~Connecting();

  virtual void OnEvent(EventDriver *poll, int fd, short event);

  void SetIp(const std::string& ip) { ip_ = ip; }
  std::string GetIp() { return ip_; }

  void SetFd(const int& fd) { fd_ = fd; }
  int GetFd() { return fd_; }

  void SetPort(const int& port) { port_ = port; }
  int GetPort() { return port_; }

  inline void SetProtocol(int cate) { protocol_ = protocol_factory_.Create(cate); }
  inline ProtocolBase* GetProtocol() { return protocol_; }

  virtual void OnRead(EventDriver *poll);
  virtual void OnWrite();

  int DecodeMsg();
  int EncodeMsg(EventMessage* message);

  void OnActiveClose();
protected:
  virtual int RegisterAddModr( int fd, bool set);
  virtual int RegisterModr(int fd, bool set);
  virtual int RegisterModw(int fd, bool set);
  virtual int RegisterDel(int fd);

  virtual void OnDecodeMessage(EventMessage* message) = 0;

  virtual void OnCloseConnection(EventDriver *poll,int fd);
  virtual void OnError(EventDriver *poll);

  void OnReadWriteClose();
  void Close(EventDriver* poll);
  void Clean();

  void Destroy() {
    delete this;
  }
  virtual void OnClose();

private:
  int fd_;
  std::string ip_;
  int port_;
  Buffer inbuf_;
  Buffer outbuf_;

  ProtocolBase* protocol_;
  ProtocolFactory protocol_factory_;

  LOGGER_CLASS_DECL(logger_);
};

}


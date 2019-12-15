#pragma once

#include "event/event_data.h"
#include "common/buffer.h"
#include "common/logger.h"
#include "protocol/protocol_base.h"

namespace bdf{

class EventDriver;
class EventMessage;

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
  inline void SetProtocol(ProtocolBase* protocol) { protocol_ = protocol; }
  inline ProtocolBase* GetProtocol() { return protocol_; }

  virtual void OnRead();
  virtual void OnWrite();

  virtual int DecodeMsg();
  virtual int EncodeMsg(EventMessage* message);

  void OnActiveClose();
  virtual void  Destroy() = 0;

  virtual void OnClose() = 0;

  //���ڿͻ���
  virtual bool IsServer() = 0;

  int64_t GetConnectId() { return connect_id_; }
  void SetConnectId(int64_t connect_id) { connect_id_ = connect_id; }

protected:
  virtual int RegisterAddModrw(int fd, bool set);
  virtual int RegisterDel(int fd);

  virtual void OnDecodeMessage(EventMessage* message) = 0;

  void RegisterDel(EventDriver *poll, int fd);

  void OnReadWriteClose();
  void Clean();

protected:
  int fd_;
  std::string ip_;
  int port_;
  Buffer inbuf_;
  Buffer outbuf_;

  ProtocolBase* protocol_;
  ProtocolFactory protocol_factory_;

  int64_t connect_id_;

  LOGGER_CLASS_DECL(logger_);
};

}


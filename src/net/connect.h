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

  virtual bool IsServer() = 0;

  // A request for this connection was dropped before it was encoded
  // (overload protection, closed socket). Lets a single-send client release
  // its busy flag. Runs on the connection's I/O thread.
  virtual void OnSendDropped() {}

  // A reply for this connection was encoded into outbuf_ (I/O thread).
  virtual void OnReplyEncoded() {}

  // Server replies carry the connection as descriptor_id. The upper 16 bits
  // hold a generation (low bits of connect_id) so a reply that outlives its
  // connection is not delivered to a new connection that happens to reuse
  // the same address. Heap pointers fit in 48 bits: on Linux, 57-bit (LA57)
  // or 52-bit arm64 addresses are only handed out to mmap calls that ask
  // for them with a high hint, which malloc never does.
  static int64_t MakeDescriptor(Connecting* con, int64_t connect_id) {
    return (int64_t)(((uint64_t)(uintptr_t)con & kDescriptorPtrMask)
      | ((uint64_t)(connect_id & 0xFFFF) << 48));
  }
  static Connecting* DescriptorConnect(int64_t descriptor) {
    return (Connecting*)(uintptr_t)((uint64_t)descriptor & kDescriptorPtrMask);
  }
  static uint16_t DescriptorGen(int64_t descriptor) {
    return (uint16_t)((uint64_t)descriptor >> 48);
  }
  static const uint64_t kDescriptorPtrMask = 0x0000FFFFFFFFFFFFULL;

  int64_t GetConnectId() { return connect_id_; }
  // Clock ms of the last successful read (I/O thread only).
  uint64_t GetLastReadMs() const { return last_read_ms_; }
  void SetConnectId(int64_t connect_id) { connect_id_ = connect_id; }

protected:
  virtual int RegisterAddModrw(int fd, bool set);
  virtual int RegisterDel(int fd);
  // Toggle EPOLLOUT on the owning I/O loop (virtual for tests).
  virtual int SetWriteInterest(bool set);
  virtual int WriteSocket(const char* data, int size);

  virtual void OnDecodeMessage(EventMessage* message) = 0;

  void RegisterDel(EventDriver *poll, int fd);

  void OnReadWriteClose();
  // The peer finished sending (read returned 0, or RDHUP). Default: deliver
  // what the EOF completes, then close.
  virtual void OnPeerClosed();
  void Clean();

protected:
  int fd_;
  bool write_interest_ = false;
  // Next Clean() closes normally (queued data is still sent) instead of
  // with SO_LINGER 0, which resets the connection and drops it.
  bool graceful_close_ = false;
  uint64_t last_read_ms_ = 0;
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


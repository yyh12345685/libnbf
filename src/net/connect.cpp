#include "net/connect.h"
#include "common/time.h"
#include "net/socket.h"
#include "event/event_driver.h"
#include "protocol/protocol_base.h"
#include "service/service_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_threads.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, Connecting);

// Pending bytes allowed for a peer that stopped reading. Above this the
// connection is closed: dropping bytes would split a frame and desync the peer.
const static size_t max_send_buf_failed_size = 64 * 1024 * 1024;

Connecting::Connecting():
  fd_(-1),
  port_(-1),
  protocol_(nullptr),
  connect_id_(-1){
}

Connecting::~Connecting(){
  if (protocol_){
    protocol_->Release();
  }

  if (fd_ > 0){
    Clean();
  }
}

void Connecting::OnEvent(EventDriver *poll, int fd, short event){
  TRACE(logger_, "Connecting::OnEvent,event:"<< event<<",fd"<<fd);
  if (fd != fd_ || fd_ < 0) {
    return;
  }
  if (event & EVENT_READ) {
    OnRead();
  }
  // Reading may close the connection. Never write to that socket again.
  if (fd != fd_) {
    return;
  }
  if (event & (EVENT_CONNECT_CLOSED | EVENT_ERROR)) {
    OnClose();
    return;
  }
  if (event & EVENT_WRITE) {
    OnWrite();
  }
}

void Connecting::OnRead(){
  if (fd_ <= 0){
    TRACE(logger_, "invalid sock:" << fd_);
    return;
  }

  TRACE(logger_, "Read data start.");
  char tmp[8 * 1024];
  int nread = 0;
  int total_read = 0;
  bool rc = true;
  while (rc){
    nread = Socket::TcpSocketRead(fd_, tmp, sizeof(tmp));
    if (nread < 0){
      if (EINTR == errno) {
        continue;
      }
      if (EAGAIN == errno || EWOULDBLOCK == errno){
        //nodata or data has all readed
        DEBUG(logger_, "TcpSocketRead errno:" << errno << ",ip:" << GetIp()
          << ",port:" << GetPort() << ",read size:" << total_read);
        break;
      } else{
        if (1 == rand() % 10)
          INFO(logger_, "TCP read failed, fd is:" << fd_ << ",ip:" << GetIp() << ",port:" 
            << GetPort()<< ",total_read:" << total_read << ",errno:" << strerror(errno)
            <<",is_server_:"<<IsServer()<<",prt:"<<this);
        OnReadWriteClose();
        return;
      }
    } else if (0 == nread){
      DEBUG(logger_, "TCP client is closed: fd=" << fd_ << ",ip:" << ip_ << ",port:" << port_);
      OnReadWriteClose();
      return;
    }

    total_read += nread;
    last_read_ms_ = Time::GetCurrentClockTime();
    if (nread == sizeof(tmp)){
      DEBUG(logger_, "continue read. fd:" << fd_);
      rc = true;
    } else{
      rc = false;
    }

    if (!inbuf_.EnsureSize(nread)){
      WARN(logger_, "Malloc failed, new more=" << nread 
        << ", old capacity=" << inbuf_.GetCapacity());
      return;
    }

    memcpy(inbuf_.GetWriting(), tmp, nread);
    inbuf_.PourWriting(nread); // true, absolutely!
  }
  TRACE(logger_, "Read data size is:" << total_read);
  if (0!= DecodeMsg()){
    OnReadWriteClose();
  }
}

int Connecting::DecodeMsg(){
  bool failed = false;
  for (;;){
    EventMessage* msg = protocol_->Decode(inbuf_, failed);
    if (failed){
      TRACE(logger_, "Connecting::Decode,base->Decode failed.");
      if (msg != nullptr){
        MessageFactory::Destroy(msg);
      }
      failed = true;
      break;
    }

    if (nullptr == msg){
      TRACE(logger_, "event is nullptr.");
      break;
    }

    OnDecodeMessage(msg);
  }
  if (failed){
    INFO(logger_, "DecodeMsg failed, fd is:" << fd_ << ",ip:" << GetIp() << ",port:"
      << GetPort() << ",is_server_:" << IsServer() << ",prt:" << this);
    return -1;
  }
  return 0;
}

void Connecting::OnWrite(){
  if (fd_ <= 0){
    TRACE(logger_, "invalid sock:" << fd_);
    return ;
  }

  int size = 0;
  int send = 0;
  while ((size = outbuf_.GetReadingSize()) > 0){
    int ret = WriteSocket(outbuf_.GetReading(), size);
    if (ret > 0){
      send += ret;
      outbuf_.DrainReading(ret);
      TRACE(logger_, "send size is:" << ret);
      continue;
    }
    if (ret < 0 && EINTR == errno) {
      continue;
    }
    if (ret < 0 && (EAGAIN == errno || EWOULDBLOCK == errno)) {
      // Socket buffer is full: keep the data and let epoll call OnWrite
      // again once the socket is writable.
      if (outbuf_.GetReadingSize() > max_send_buf_failed_size) {
        WARN(logger_, "peer not reading, pending:" << outbuf_.GetReadingSize()
          << ",ip:" << ip_ << ",port:" << port_ << ",is_server_:" << IsServer());
        OnReadWriteClose();
        return;
      }
      if (!write_interest_) {
        if (0 != SetWriteInterest(true)) {
          OnReadWriteClose();
          return;
        }
        write_interest_ = true;
      }
      TRACE(logger_, "send would block, pending:" << outbuf_.GetReadingSize());
      return;
    }
    if (1 == rand() % 10)
      INFO(logger_, "errno:" << errno << ",TCP write failed,send:" << send
        << ",ret:" << ret << ",ip:" << GetIp() << ",port:" << GetPort()
        << ",is_server_:" << IsServer() << ",fd:" << fd_ << ",ptr:" << this);
    //发现偶尔会触发两次，一次errno104，一次errno32
    OnReadWriteClose();
    return;
  }
  // Everything is out: stop waking up for EPOLLOUT.
  if (write_interest_) {
    if (0 != SetWriteInterest(false)) {
      OnReadWriteClose();
      return;
    }
    write_interest_ = false;
  }
  TRACE(logger_, "Real Send Message size is:" << send);
}

int Connecting::SetWriteInterest(bool set){
  return service::GetServiceManager().GetNetThreadManager()
    ->GetIoThreads()->Modw(this, fd_, set);
}

int Connecting::WriteSocket(const char* data, int size){
  return Socket::TcpSocketWrite(fd_, const_cast<char*>(data), size);
}

int Connecting::EncodeMsg(EventMessage* message){
  //only used in send server response
  if (unlikely(!protocol_->Encode(message, &outbuf_))){
    //allway not to here
    WARN(logger_, "Connecting::EncodeMsg error.");
    return -1;
  }
  
  return 0;
}

int Connecting::RegisterAddModrw(int fd, bool set){
  WARN(logger_, "not should to here Connecting::RegisterAddModrw.");
  return 0;
}

int Connecting::RegisterDel(int fd){
  WARN(logger_, "not should to here Connecting::RegisterDel.");
  return 0;
}

void Connecting::OnActiveClose(){
  if (fd_ < 0) {
    return;
  }

  if (0 != RegisterDel(fd_)) {
    TRACE(logger_, "Connecting::Close,poll_->Del error,sock:" << fd_);
  }
}

//when read write
void Connecting::OnReadWriteClose(){
  if (fd_ >= 0) {
    OnClose();
  }
}

void Connecting::Clean(){
  if (!Socket::SetSoLinger(fd_, 0)){
    TRACE(logger_, "SocketHelper::SetSoLinger failed.");
  }

  Socket::Close(fd_);
  fd_ = -1;
  write_interest_ = false;

  inbuf_.ResetAll();
  outbuf_.ResetAll();
  if (protocol_) {
    protocol_->Reset();
  }
}

void Connecting::RegisterDel(EventDriver *poll, int fd){
  TRACE(logger_, "Connecting::RegisterDel,sock:" << fd_
    << ",ip:" << ip_ << ",port:" << port_<<",con id:"<<connect_id_);
  if (fd_ < 0 || nullptr == poll) {
    return;
  }

  if (0 != poll->Del(fd_)) {
    TRACE(logger_, "Connecting::Close,poll_->Del error,sock:" << fd_);
  }
}

}

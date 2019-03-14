#include "net/connect.h"
#include "net/socket.h"
#include "event/event_driver.h"
#include "protocol/protocol_base.h"
#include "service/io_service.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, Connecting);

const static int max_send_buf_failed_size = 10 * 1024 * 1024+1;//10M

Connecting::Connecting()
  :fd_(-1)
  ,protocol_(nullptr){
}

Connecting::~Connecting(){
  if (protocol_){
    protocol_->Release();
  }
}

void Connecting::OnEvent(EventDriver *poll, int fd, short event){
  if (event & EVENT_CONNECT_CLOSED){
    OnCloseConnection(poll,fd);
    return;
  }

  if (event & EVENT_READ){
    OnRead(poll);
  }
  if (event & EVENT_WRITE){
    OnWrite();
  }
  if (event & EVENT_ERROR){
    TRACE(logger_, "Connecting::OnEvent,EVENT_ERROR.");
    OnError(poll);
  }
}

void Connecting::OnRead(EventDriver *poll){
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
      if (EAGAIN == errno || EWOULDBLOCK == errno){
        //nodata or data has all readed
        INFO(logger_, "TcpSocketRead errno:" << errno << ",ip:" << GetIp() 
          << ",port:" << GetPort() << ",read size:" << total_read);
        break;
      } else{
        INFO(logger_, "TCP read failed, fd is:" << fd_ << ",ip:" << GetIp() << ",port:" 
          << GetPort()<< ",total_read:" << total_read << ",errno:" << strerror(errno));
        Close(poll);
        return;
      }
    } else if (0 == nread){
      DEBUG(logger_, "TCP client is closed: fd=" << fd_ << ",ip:" << ip_ << ",port:" << port_);
      this->Close(poll);
      return;
    }

    total_read += nread;
    if (nread == sizeof(tmp)){
      DEBUG(logger_, "continue read. fd:" << fd_);
      rc = true;
    } else{
      rc = false;
    }

    if (!inbuf_.EnsureSize(nread)){
      WARN(logger_, "Malloc failed, new more=" << nread << ", old capacity=" << inbuf_.GetCapacity());
      return;
    }

    memcpy(inbuf_.GetWriting(), tmp, nread);
    inbuf_.PourWriting(nread); // true, absolutely!
  }
  TRACE(logger_, "Read data size is:" << total_read);
  if (0!= DecodeMsg()){
    Close(poll);
  }
}

int Connecting::DecodeMsg(){
  bool failed = false;
  for (;;){
    EventMessage* msg = protocol_->Decode(inbuf_, failed);
    if (failed){
      TRACE(logger_, "Connecting::Decode,base->Decode failed.");
      if (msg != NULL){
        MessageFactory::Destroy(msg);
      }
      break;
    }

    if (NULL == msg){
      TRACE(logger_, "event is NULL.");
      break;
    }

    OnDecodeMessage(msg);
  }
  if (failed){
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
  int should_send_size = outbuf_.GetReadingSize();
  while ((size = outbuf_.GetReadingSize()) > 0){
    int ret = Socket::TcpSocketWrite(fd_, outbuf_.GetReading(), size);
    if (ret > 0){
      send += ret;
      outbuf_.DrainReading(ret);
      if (send >= should_send_size){
        RegisterModw(fd_, false);
        break;
      }
      TRACE(logger_, "send size is:" << ret);
    } else if (ret < 0){
      if (errno == EINTR || errno == EAGAIN){

      }else{
        INFO(logger_, "errno:" << errno << ",TCP write failed:" << ",send:" << send<< ",ret" << ret 
          << ",ip:" << GetIp() << ",port:" << GetPort() << ",send capacity" << outbuf_.GetCapacity());
        OnReadWriteClose();
        break;
      }

      if (send > 0){
        INFO(logger_, "send size:" << send << ",not send size:" << (should_send_size - send)
          << ",send capacity:" << outbuf_.GetCapacity() << ",not send completed,errno:" << errno);
        if (outbuf_.GetReadingSize() > max_send_buf_failed_size){
          outbuf_.DrainReading(outbuf_.GetReadingSize());
          TRACE(logger_, "will be lose size:" << (should_send_size - send));
        } else{
          // this operation will let the event poll to call the function habdleWrite when the write-cache enable to write 
          RegisterModw(fd_, true);
        }
      } else{
        TRACE(logger_, "socket not ok or server is cannot handled,errno" << errno);
        outbuf_.DrainReading(outbuf_.GetReadingSize());
      }
      break;
    } else{
      INFO(logger_, "send return zero,TCP write failed,fd=:" << fd_ 
        << ",errno:" << errno << ",ip:" << ip_ << ",port:" << port_);
      OnReadWriteClose();
      break;
    }
  }
  TRACE(logger_, "Real Send Message size is:" << send);
}

int Connecting::EncodeMsg(EventMessage* message){

  if (!protocol_->Encode(message, &outbuf_)){
    //allway not to here
    WARN(logger_, "Connecting::EncodeMsg error.");
    return -1;
  }
  return 0;
}

int Connecting::RegisterAddModr(int fd, bool set) {
  WARN(logger_, "not should to here Connecting::AddModr.");
  return 0;
}

int Connecting::RegisterDel(int fd){
  WARN(logger_, "not should to here Connecting::RegisterDel.");
  return 0;
}

int Connecting::RegisterModr(int fd, bool set){
  WARN(logger_, "not should to here Connecting::Modr.");
  return 0;
}

int Connecting::RegisterModw(int fd, bool set){
  WARN(logger_, "not should to here Connecting::Modw.");
  return 0;
}
//when read
void Connecting::Close(EventDriver* poll){
  TRACE(logger_, "Connecting::Close,sock:" << fd_ 
    << ",ip:" << ip_ << ",port:" << port_);
  if (fd_ < 0 || nullptr == poll){
    return ;
  }

  if (0 != poll->Del(fd_)){
    TRACE(logger_, "Connecting::Close,poll_->Del error,sock:" << fd_);
  }

  Clean();
}

void Connecting::OnActiveClose(){
  if (fd_ < 0) {
    return;
  }

  if (0 != RegisterDel(fd_)) {
    TRACE(logger_, "Connecting::Close,poll_->Del error,sock:" << fd_);
  }
}

void Connecting::OnReadWriteClose(){
  OnActiveClose();
  Clean();
}

void Connecting::OnClose(){
  Destroy();
}

void Connecting::Clean(){
  if (!Socket::SetSoLinger(fd_, 0)){
    TRACE(logger_, "SocketHelper::SetSoLinger failed.");
  }

  Socket::Close(fd_);
  fd_ = -1;

  inbuf_.ResetAll();
  outbuf_.ResetAll();
}

void Connecting::OnCloseConnection(EventDriver *poll,int fd){
  TRACE(logger_, "client close fd:" << fd);
  if (fd != fd_){
    WARN(logger_, "sock may be not inited,fd:" << fd << ",sock:" << fd_);
  }
  Close(poll);
  OnClose();
}

void Connecting::OnError(EventDriver *poll){
  if (fd_ > 0){
    INFO(logger_, "HandleError close fd:" << fd_ << ",ip:" << ip_ << ",port:" << port_);
    Close(poll);
    OnClose();
  }
}

}

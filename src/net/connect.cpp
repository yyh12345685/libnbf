#include "net/connect.h"
#include "net/socket.h"
#include "event/event_driver.h"
#include "protocol/protocol_base.h"
#include "service/io_service.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, Connecting);

const static int max_send_buf_failed_size = 10 * 1024 * 1024+1;//10M

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
  if ((event & EVENT_CONNECT_CLOSED) || (event & EVENT_ERROR)){
    TRACE(logger_, "Connecting::OnEvent,OnError event:"<< event);
    //do not close socket ,error message allways hava read message for close
    if (IsServer()){
      RegisterDel(poll, fd);
    }
  }

  bdf::EventMessage* message = bdf::MessageFactory::Allocate<bdf::EventMessage>(0);
  message->descriptor_id = (int64_t)((void*)this);
  message->event_mask = event;
  IoService::GetInstance().SendEventToIoHandle(message);
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
  int should_send_size = outbuf_.GetReadingSize();
  while ((size = outbuf_.GetReadingSize()) > 0){
    int ret = Socket::TcpSocketWrite(fd_, outbuf_.GetReading(), size);
    if (ret > 0){
      send += ret;
      outbuf_.DrainReading(ret);
      if (send >= should_send_size){
        //RegisterModw(fd_, false);
        break;
      }
      TRACE(logger_, "send size is:" << ret);
    } else if (ret < 0){
      if (EINTR == errno) {
        continue;
      }
      if (errno == EINTR || errno == EAGAIN){

      }else{
        if (1 == rand() % 10)
          INFO(logger_, "errno:" << errno << ",TCP write failed:" << ",send:" << send
            << ",ret:" << ret << ",ip:" << GetIp() << ",port:" << GetPort()
            <<",is_server_:"<<IsServer()<< ",send capacity:" << outbuf_.GetCapacity()
            <<",fd:"<<fd_ << ",errno:" << strerror(errno)<<",ptr:"<<this);
        //发现偶尔会触发两次，一次errno104，一次errno32
        OnReadWriteClose();
        break;
      }

      if (send > 0){
        INFO(logger_, "send size:" << send << ",not send size:" << (should_send_size - send)
          << ",send capacity:" << outbuf_.GetCapacity() << ",not send completed,errno:" << errno
          <<",ip:"<<ip_<<",port:"<<port_<<",is_server_:"<<IsServer());
        if (outbuf_.GetReadingSize() > max_send_buf_failed_size){
          outbuf_.DrainReading(outbuf_.GetReadingSize());
          TRACE(logger_, "will be lose size:" << (should_send_size - send));
        } else{
          // this operation RegisterModw will let the event poll to call the function handleWrite 
          // when the write-cache enable to write 
          //RegisterModw(fd_, true);
        }
      } else{
        TRACE(logger_, "socket not ok or server is cannot handled,errno" << errno);
        outbuf_.DrainReading(outbuf_.GetReadingSize());
      }
      break;
    } else{
      INFO(logger_, "send return zero,TCP write failed,fd=:" << fd_ 
        << ",errno:" << errno << ",ip:" << ip_ << ",port:" << port_ << ",ptr:" << this);
      OnReadWriteClose();
      break;
    }
  }
  TRACE(logger_, "Real Send Message size is:" << send);
}

int Connecting::EncodeMsg(EventMessage* message){
  //only used in send server response
  if (!protocol_->Encode(message, &outbuf_)){
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
  //events is delete in (event & EVENT_CONNECT_CLOSED) || (event & EVENT_ERROR)
  Socket::ShutDownBoth(fd_);
  //OnClose();
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

void Connecting::RegisterDel(EventDriver *poll, int fd){
  TRACE(logger_, "Connecting::RegisterDel,sock:" << fd_
    << ",ip:" << ip_ << ",port:" << port_);
  if (fd_ < 0 || nullptr == poll) {
    return;
  }

  if (0 != poll->Del(fd_)) {
    TRACE(logger_, "Connecting::Close,poll_->Del error,sock:" << fd_);
  }
}

}

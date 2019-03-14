
#include "event/event_notifier.h"
#include "event/event_driver.h"
#include "net/socket.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, WakeUpFd);

void WakeUpFd::OnEvent(EventDriver *poll, int fd, short event){
  //TRACE(logger_, "WakeUpFd::WakeUpFd::OnEvent.");
  char buf[32];
  if (-1 == read(fd, buf, sizeof(buf))){
    WARN(logger_, "read is failed, errno:" << errno << ",fd:" << fd);
  }
}

LOGGER_CLASS_IMPL(logger_, EventNotifier);

EventNotifier::~EventNotifier() {
  if (wake_){
    event_driver_->Del(wake_fd_[0]);
    event_driver_->Del(wake_fd_[1]);
    close(wake_fd_[0]);
    close(wake_fd_[1]);
    delete wake_;
    wake_fd_[0] = wake_fd_[1] = -1;
    wake_ = nullptr;
  }
}

int EventNotifier::InitWakeup(){

  if (pipe(wake_fd_) == -1)
    return -1;

  if (!Socket::SetNonBlock(wake_fd_[0])
    || !Socket::SetNonBlock(wake_fd_[1])){
    Socket::Close(wake_fd_[0]);
    Socket::Close(wake_fd_[1]);
    return -1;
  }

  WakeUpFd *wfd = new WakeUpFd();
  if (event_driver_->Add(wake_fd_[0], wfd, true) == -1){
    delete wfd;
    Socket::Close(wake_fd_[0]);
    Socket::Close(wake_fd_[1]);
    return -1;
  }
  wake_ = wfd;
  if (0 != event_driver_->Modr(wake_fd_[0], true)){
    WARN(logger_, "InitWakeup failed,wake_fd_[0]:"<< wake_fd_[0]);
  }
  return 0;
}

int EventNotifier::Wakeup()
{
  //TRACE(logger_, "EventDriver::Wakeup.");
  int ret = write(wake_fd_[1], "", 1);
  if (ret == 1 || errno == EAGAIN)
    return 0;
  return -1;
}

}


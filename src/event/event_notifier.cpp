
#include <sys/eventfd.h>
#include "event/event_notifier.h"
#include "event/event_driver.h"
#include "net/socket.h"
#include "common/thread_id.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, WakeUpFd);

void WakeUpFd::OnEvent(EventDriver *poll, int fd, short event){
  TRACE(logger_, "WakeUpFd::WakeUpFd::OnEvent.");
  uint64_t value = 1;
  int ret = read(fd, &value, sizeof(value));
  if (ret != sizeof(value)){
    WARN(logger_, "read failed, errno:" << errno << ",fd:" << fd <<",ret:" << ret);
  }
}

LOGGER_CLASS_IMPL(logger_, EventNotifier);

EventNotifier::~EventNotifier() {
  if(-1 != event_fd_){
    event_driver_->Del(event_fd_);
    close(event_fd_);
    event_fd_ = -1;
  }
  if (wake_){
    delete wake_;
    wake_ = nullptr;
  }
}

int EventNotifier::InitWakeup(){
  if (event_fd_ != -1) {
    return 0;
  }

  int event_fd = -1;
  if (-1 == (event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))) {
    WARN(logger_, "EventNotifier::InitWakeup fail errno:" << errno);
    return -1;
  }

  WakeUpFd *wfd = new WakeUpFd();
  if (event_driver_->Add(event_fd, wfd, true) == -1){
    WARN(logger_, "InitWakeup add failed,event_fd:"<< event_fd);
    delete wfd;
    Socket::Close(event_fd);
    return -1;
  }
  wake_ = wfd;
  if (0 != event_driver_->Modr(event_fd, true)){
    WARN(logger_, "InitWakeup modr failed,event_fd:"<< event_fd);
  }
  event_fd_ = event_fd;
  TRACE(logger_, "InitWakeup ok,event_fd_:"<<event_fd_);
  return 0;
}

int EventNotifier::Wakeup(){
  TRACE(logger_, "EventDriver::Wakeup.");
  if(event_thread_id_ == ThreadId::Get()){
    TRACE(logger_, "event_thread_id_ == ThreadId::Get().");
    return 0;
  }
  if(-1 == event_fd_){
    TRACE(logger_, "event_fd_ not inited.");
    return -1;
  }
  
  uint64_t value = 1;
  int ret = write(event_fd_, &value, sizeof(uint64_t));
  if(ret != sizeof(uint64_t)){
    WARN(logger_, "EventNotifier::Wakeup fail errno:" 
      << errno << ",ret:" << ret << ",fd"<< event_fd_);
    return -2;
  }
  return 0;
}

}


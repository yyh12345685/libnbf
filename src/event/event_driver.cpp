
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "net/socket.h"
#include "event/event_driver.h"
#include "thread_data_run.h"
#include "common/thread_id.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, EventDriver);

EventDriver::EventDriver() :
  run_(true),
  event_in_(EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP),
  event_out_(EPOLLOUT | EPOLLHUP | EPOLLERR | EPOLLRDHUP),
  event_notifier_(this),
  thread_data_run_(nullptr){
  epfd_ = -1;
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  sigprocmask(SIG_BLOCK, &set, nullptr);
  
	event_in_ |= EPOLLET;
	event_out_ |= EPOLLET;
    
  event_data_.InitEventData();
}

EventDriver::~EventDriver(){
	ShutDown();
}

int EventDriver::Run(){
  event_notifier_.SetEventThreadId(ThreadId::Get());
  while (run_){
    this->Poll(1);
    if(thread_data_run_!=nullptr){
      thread_data_run_->CallRun();
    }
  }
  this->Poll(0); // clear unproc tasks
  if(thread_data_run_!=nullptr){
    // Run queued close/destroy messages posted right before Stop, otherwise
    // those connections are never destroyed.
    thread_data_run_->CallRun();
  }
  return 0;
}

int EventDriver::Stop(){
  run_ = false;
  return 0;
}

int EventDriver::Init(){
  maxfd_ = -1;
  inloop_ = false;

  epfd_ = epoll_create(10240);
  if (0 != event_notifier_.InitWakeup())
  {
    WARN(logger_, "EventDriver::Init InitWakeup() error.");
  }
  return epfd_;
}

int EventDriver::ShutDown()
{
  this->Poll(0); // only for handle the left timeout timers/events once
  
  maxfd_ = -1;
  inloop_ = false;
  close(epfd_);
  epfd_ = -1;
  return 0;
}

int EventDriver::Add(int fd, EventFunctionBase *ezfd,bool lock, short events){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  if (fd > 0 && fd < event_data_.fd2data_size_ 
    && nullptr != event_data_.fd2data_[fd]){
    WARN(logger_, "repeated add fd:" << fd);
    return -1;
  }

  TRACE(logger_, "fd:"<<fd<<",ezfd:"<<ezfd<<",epfd:"<<epfd_<<",maxfd_"<<maxfd_);
  FdEvent *data = new FdEvent();
  data->fd_ = fd;
  data->ezfd_ = ezfd;
  data->event_ = events & (EVENT_READ | EVENT_WRITE);
  data->closed_ = false;
  if (fd > maxfd_){
    if (fd > event_data_.fd2data_size_ 
      && !event_data_.ReInitEventData(fd)){
      delete data;
      return -2;
    }
    maxfd_ = fd;
  }
  event_data_.fd2data_[fd] = data;

  struct epoll_event event = { 0 };
  event.data.ptr = (void *)(data);
  event.events = (data->event_ & EVENT_READ ? event_in_ : 0)
    | (data->event_ & EVENT_WRITE ? event_out_ : 0);
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &event) == -1){
    WARN(logger_, "EventDriver::Add epoll_ctl error.");
    delete data;
    event_data_.fd2data_[fd] = nullptr;
    return -3;
  }
  return 0;
}

int EventDriver::Del(int fd){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  if(nullptr == event_data_.fd2data_
      || fd < 0
      || fd > maxfd_
      || nullptr == event_data_.fd2data_[fd]){
    WARN(logger_, "EventDriver::Del fd:"<<fd<<",maxfd:"<<maxfd_);
    return -2;
  }

  // Always defer the delete to the end of the loop's Poll (RemoveClosed):
  // Del may run on another thread while Poll holds this FdEvent from
  // epoll_wait. (Deleting at once when !inloop_ raced with that.)
  epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
  if (!event_data_.ReInitClosed(fd)) {
    event_data_.fd2data_[fd]->closed_ = true; // leak rather than use-after-free
  }
  event_data_.fd2data_[fd] = nullptr;
  Wakeup();
  return 0;
}

int EventDriver::Modr(int fd, bool set){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  if (fd < 0 || fd > maxfd_ || nullptr == event_data_.fd2data_[fd]){
    TRACE(logger_, "fd is invalid or fd2data_ is nullptr, fd" << fd << ",maxfd" << maxfd_);
    return -1;
  }

  if (set) {
    event_data_.fd2data_[fd]->event_ |= EVENT_READ;
  } else {
    event_data_.fd2data_[fd]->event_ &= ~EVENT_READ;
  }
  Wakeup();
  int ret = EventDriver::Mod(fd);
  return ret;
}

int EventDriver::Modw(int fd, bool set){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  if (fd < 0 || fd > maxfd_ || nullptr == event_data_.fd2data_[fd]){
		TRACE(logger_,"fd2data_ is nullptr, fd"<<fd<<",maxfd"<<maxfd_);
    return -1;
  }

  if (set) {
    event_data_.fd2data_[fd]->event_ |= EVENT_WRITE;
  } else {
    event_data_.fd2data_[fd]->event_ &= ~EVENT_WRITE;
  }
  Wakeup();
  int ret= EventDriver::Mod(fd);
  return ret;
}

int EventDriver::Modrw(int fd, bool set){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  if (fd < 0 || fd > maxfd_ || nullptr == event_data_.fd2data_[fd]) {
    TRACE(logger_, "fd2data_ is nullptr, fd" << fd << ",maxfd" << maxfd_);
    return -1;
  }

  if (set) {
    event_data_.fd2data_[fd]->event_ |= EVENT_WRITE;
    event_data_.fd2data_[fd]->event_ |= EVENT_READ;
  } else {
    event_data_.fd2data_[fd]->event_ &= ~EVENT_WRITE;
    event_data_.fd2data_[fd]->event_ &= ~EVENT_READ;
  }
  Wakeup();
  int ret = EventDriver::Mod(fd);
  return ret;
}

int EventDriver::Poll(int timeout){
  //process timer
  timer_.ProcessTimer();
  //process event
  struct epoll_event events[4096];
  int numfd = epoll_wait(epfd_, events, 4096, timeout);
  if (numfd <= 0)
    return 0;
  inloop_ = true;
  for (int i = 0; i < numfd; ++i){
    FdEvent *data = (FdEvent *)events[i].data.ptr;
    if(nullptr == data){
      WARN(logger_, "nullptr == data...");
      continue;
    }
    // Snapshot under the lock; another thread may Del/Mod this fd.
    EventFunctionBase *ezfd = nullptr;
    short registered = EVENT_NONE;
    int data_fd = -1;
    {
      std::lock_guard<std::recursive_mutex> guard(fd_lock_);
      if (data->closed_) {
        continue;
      }
      ezfd = data->ezfd_;
      registered = data->event_;
      data_fd = data->fd_;
    }
    bool expected_event = false;
    short event = EVENT_NONE;
    if ((registered & EVENT_READ) && (events[i].events & EPOLLIN)){
      event |= EVENT_READ;
      expected_event = true;
    }
    if ((registered & EVENT_WRITE) && (events[i].events & EPOLLOUT)){
      event |= EVENT_WRITE;
      expected_event = true;
    }
    if (events[i].events & EPOLLRDHUP){
      event |= EVENT_CONNECT_CLOSED;
      expected_event = true;
    }
    if (events[i].events & (EPOLLERR | EPOLLHUP)){
      event |= EVENT_ERROR;
      expected_event = true;
    }
    if (!expected_event){
      WARN(logger_, "unexpected event........");
      continue;
    }
    ezfd->OnEvent(this, data_fd, event);
  }

  inloop_ = false;
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  event_data_.RemoveClosed();
  return 0;
}

int EventDriver::Wakeup(){
  return event_notifier_.Wakeup();
}

int EventDriver::Mod(int fd){
  std::lock_guard<std::recursive_mutex> guard(fd_lock_);
  FdEvent *data = event_data_.fd2data_[fd];
  if(nullptr == data){
    WARN(logger_, "nullptr == data...");
    return -1;
  }
  struct epoll_event event = {0};
  event.data.ptr = data;
  event.events = (data->event_ & EVENT_READ ? event_in_ : 0) 
    | (data->event_ & EVENT_WRITE ? event_out_ : 0);
  if ( 0!= epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &event)){
    return -2;
  }
  return 0;
}

}

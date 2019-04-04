
#include <string.h>
#include "event/event_data.h"

namespace bdf {

static int incr_fd2data = 4096;
static int init_fd2data = 40960;

LOGGER_CLASS_IMPL(logger_, EventData);

bool EventData::InitEventData(){
  fd2data_ = (FdEvent **)malloc((init_fd2data + 1) * sizeof(*fd2data_));

  if (nullptr == fd2data_){
    WARN(logger_, "Malloc error.");
  } else{
    TRACE(logger_, "Malloc ok.");
    memset(fd2data_, 0, sizeof(*fd2data_) *(init_fd2data + 1));
    fd2data_size_ = init_fd2data;
  }

  closed_ = nullptr;
  closed_size_ = closed_count_ = 0;
  return true;
}

bool EventData::ReInitEventData(int& fd){
  INFO(logger_, "before FdEvent realloc, fd2data_size_:" << fd2data_size_);

  int data_size_tmp = fd2data_size_ + incr_fd2data;

  while (data_size_tmp < fd)
    data_size_tmp += incr_fd2data;

  FdEvent **tmp = (FdEvent **)realloc(fd2data_, (data_size_tmp + 1) * sizeof(*tmp));
  if (!tmp){
    WARN(logger_, "EventData::Add realloc error.");
    return false;
  }
  fd2data_ = tmp;
  memset(fd2data_ + fd2data_size_ + 1, 0, sizeof(*fd2data_) * (data_size_tmp - fd2data_size_));
  fd2data_size_ = data_size_tmp;

  INFO(logger_, "after FdEvent realloc, fd2data_size_:" << fd2data_size_);
  return true;
}

bool EventData::ReInitClosed(int& fd){
  lock_.Lock();
  if (closed_count_ >= closed_size_){
    FdEvent **tmp = (FdEvent **)realloc(closed_, (closed_size_ + 1) * sizeof(*tmp));
    if (!tmp){
      lock_.UnLock();
      return false;
    }
    closed_ = tmp;
    closed_size_++;
  }
  if (nullptr != fd2data_[fd]){
    closed_[closed_count_++] = fd2data_[fd];
    fd2data_[fd]->closed_ = true;
  }
  lock_.UnLock();
  return true;
}

void EventData::RemoveClosed(){
  lock_.Lock();
  for (int i = 0; i < closed_count_; ++i){
    if (closed_[i]->closed_){
      delete closed_[i];
      closed_[i] = nullptr;
    }
  }
  closed_count_ = 0;
  lock_.UnLock();
}

EventData::~EventData(){
  lock_.Lock();
  if (nullptr != fd2data_){
    for (int fd = 0; fd <= fd2data_size_; ++fd){
      if (nullptr != fd2data_[fd]){
        delete fd2data_[fd];
        fd2data_[fd] = nullptr;
      }
    }
    free(fd2data_);
    fd2data_ = nullptr;
  }

  for (int i = 0; i < closed_count_; ++i){
    if (nullptr != closed_[i]){
      delete closed_[i];
      closed_[i] = nullptr;
    }
  }
  free(closed_);
  closed_ = nullptr;
  closed_count_ = closed_size_ = 0;

  lock_.UnLock();
}

}

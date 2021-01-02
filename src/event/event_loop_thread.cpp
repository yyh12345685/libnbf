
#include <functional>
#include <sys/prctl.h>
#include "event/event_loop_thread.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, EventLoopThread);

EventLoopThread::EventLoopThread() :
  is_run_(false),
  thread_(nullptr){
  poll_.Init();
}

EventLoopThread::~EventLoopThread(){
}

int EventLoopThread::Start(){
  thread_ = new std::thread(std::bind(&EventLoopThread::Run, this));
  is_run_ = true;
  return 0;
}

int EventLoopThread::Stop(){
  poll_.Stop();

  if (!thread_) {
    return 0;
  }
  
  is_run_ = false;
  thread_->join();
  delete thread_;
    
  return 0;
}

void* EventLoopThread::Run(void *arg){
  prctl(PR_SET_NAME, "EventLoopThread");
  EventLoopThread *t = (EventLoopThread *)arg;
  t->Main();
  INFO(logger, "EventLoopThread Stop.");
  return nullptr;
}

void EventLoopThread::Main(){
  poll_.Run();
}

int EventLoopThread::Add(int fd, EventFunctionBase *ezfd, bool lock) {
  return poll_.Add(fd, ezfd, lock);
}
int EventLoopThread::Del(int fd) {
  return poll_.Del(fd);
}
int EventLoopThread::Modr(int fd, bool set) {
  return poll_.Modr(fd, set);
}
int EventLoopThread::Modw(int fd, bool set) {
  return poll_.Modw(fd, set);
}

int EventLoopThread::Modrw(int fd, bool set){
  return poll_.Modrw(fd, set);
}

int EventLoopThread::Wakeup(){
  return poll_.Wakeup();
}

}

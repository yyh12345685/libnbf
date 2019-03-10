
#include "event/event_loop_thread.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, EventLoopThread);

EventLoopThread::EventLoopThread() :
  is_run_(false){
  poll_.Init();
}

EventLoopThread::~EventLoopThread(){
  //poll_.ShutDown();
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
  EventLoopThread *t = (EventLoopThread *)arg;
  t->Main();
  return nullptr;
}

void EventLoopThread::Main(){
  poll_.Run();
}

}


#pragma once

#include <vector>
#include <memory>
#include "common/common.h"
#include "common/logger.h"
#include "event/timer/timer_base.h"

namespace bdf {

struct ServiceConfig;
class AcceptorThread;
class IoThreads;
class EventFunctionBase;
class EventMessage;
class ServerConnect;

class NetThreadManager {
public:
  NetThreadManager(const ServiceConfig* conf);

  bool Init();
  bool Start();

  void Stop();
  virtual ~NetThreadManager();

  AcceptorThread* GetAcceptThread()const{ return acceptor_thread_; }
  IoThreads* GetIoThreads()const{ return io_threads_; }

  int AddModrw(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int AddModr(
    EventFunctionBase *ezfd, int fd, bool set, bool lock,int& register_thread_id);
  int Del(EventFunctionBase* ezfd, int fd);

  void ReleaseServerCon(ServerConnect* svr_con);

  void PutMessageToHandle(EventMessage* msg);
  
  uint64_t StartTimer(TimerData& data,int io_thread_id);
  void CancelTimer(uint64_t timer_id,int io_thread_id);
private:
  NetThreadManager();

  LOGGER_CLASS_DECL(logger_);

  const ServiceConfig* conf_;
  AcceptorThread* acceptor_thread_;
  IoThreads* io_threads_; 

  DISALLOW_COPY_AND_ASSIGN(NetThreadManager)
};

}


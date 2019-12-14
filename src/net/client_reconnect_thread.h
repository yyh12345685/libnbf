#pragma once

#include <thread>
#include <mutex>
#include "event/timer/timer.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"
#include "common/common.h"

namespace bdf{
//启动一个线程，发送心跳包，并且进行客户端重连操作

class ClientReconnect{
public:

  static ClientReconnect& GetInstance() {
    static ClientReconnect inst;
    return inst;
  }

  bool StartThread();
  void StopThread();

  void Run();

  uint64_t StartTimer(TimerData& data) {
    if (!is_running_){
      return 0;
    }
    lock_.lock();
    uint64_t ret = timer_.AddTimer(data.time_out_ms, data);
    lock_.unlock();
    return ret;
  }

  void CancelTimer(uint64_t timer_id) {
    lock_.lock();
    timer_.DelTimer(timer_id);
    lock_.unlock();
  }

private:
  std::thread* client_thread_ = nullptr;
  bool is_running_ = false;

  std::mutex lock_;

  Timer& GetTimer() { return timer_; }
  Timer timer_;

  LOGGER_CLASS_DECL(logger_);

  DISALLOW_COPY_ASSIGN_CONSTRUCTION(ClientReconnect)
};

}

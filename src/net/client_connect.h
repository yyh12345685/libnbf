#pragma once

#include "net/connect.h"
#include "event/timer/timer_base.h"
#include "common/logger.h"

namespace bdf{

//class  HeartBeatTimer:public OnTimerBase{
//public:
//  virtual void OnTimer(void* function_data);
//};

class ClientConnect:public Connecting,public OnTimerBase {
public:
  //friend class HeartBeatTimer;
  enum {
    kConnecting = 0,
    kConnected,
    kBroken,
    kStopping,
  };

  ClientConnect(uint32_t timeout_ms, uint32_t heartbeat_ms);
  virtual ~ClientConnect();

  virtual void OnTimer(void* function_data);

  int TryConnect();
  int Stop();

  int RegisterAddModrw(int fd,int& register_thread_id);
  int RegisterAddModr(int fd,int& register_thread_id);
  int RegisterDel(int fd);

  int GetRegisterThreadId(){
    return register_thread_id_;
  }

  inline int GetStatus() const { return status_; }
  inline void SetStatus(int status) { status_ = status; }

  virtual void OnClose();

  //这几个timer相关的线程，只有iohandle回调的时候才能调用
  int StartReconnectTimer();
  int StartHeartBeatTimer();
  void CancelTimer();

  virtual void SetBuzy(bool is_buzy) = 0;
  virtual bool TrySetBuzy() = 0;

protected:
  enum {
    kUnKown = 0,
    kClientTimerTypeReconnect = 1,
    kClientTimerTypeHeartBeat = 2,
  };
  uint32_t client_timer_type_reconnect_ = kClientTimerTypeReconnect;
  uint32_t client_timer_type_heartbeat_ = kClientTimerTypeHeartBeat;

  void OnWrite();
  void OnConnectWrite();
  bool IsConnected(uint32_t time_ms, int wait_time = 1);

  virtual void OnHeartBeat();
  virtual void CleanSequenceQueue() = 0;

  void DoSendBack(EventMessage* message, int status);
  void CleanClient();

  bool is_buzy_;
private:
  LOGGER_CLASS_DECL(logger);

  int status_ ;

  uint32_t timeout_ms_;
  uint32_t heartbeat_ms_;

  uint64_t reconnect_timer_;
  uint64_t heartbeat_timer_;

  int register_thread_id_;

  //HeartBeatTimer heart_beat_timer_;
};


}
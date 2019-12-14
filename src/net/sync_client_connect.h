#pragma once

#include "net/client_connect.h"
#include "net/sync_sequence.h"
#include "common/logger.h"

namespace bdf{

class SyncClientConnect:public ClientConnect{

public:
  SyncClientConnect(
    const uint32_t& timeout_ms, 
    const uint32_t& heartbeat_ms, 
    const bool& sigle_send_sigle_recv);

  ~SyncClientConnect();

  virtual void Destroy() {
    BDF_DELETE(this);
  }

  virtual void OnDecodeMessage(EventMessage* message);
  virtual int EncodeMsg(EventMessage* message);

  virtual void CleanSequenceQueue();

  virtual bool IsServer() { return false; }
  void CleanSyncClient();

  //仅仅用于单发单收
  virtual void SetBuzy(bool is_buzy) {
    if (!sigle_send_sigle_recv_) {
      return;
    }
    std::lock_guard<std::mutex> lock_guard(buzy_lock_);
    is_buzy_ = is_buzy;
  };

  virtual bool TrySetBuzy(){
    if (!sigle_send_sigle_recv_) {
      return true;
    }

    std::lock_guard<std::mutex> lock_guard(buzy_lock_);
    if (is_buzy_){
      return false;
    }
    is_buzy_ = true;
    return true;
  }

protected:
  virtual int DecodeMsg();
  
private:
  SyncSequence sync_sequence_;

  bool sigle_send_sigle_recv_;

  std::mutex buzy_lock_;

  //std::mutex test_lock_;
  //std::mutex test_lock1_;

  LOGGER_CLASS_DECL(logger);
};

}
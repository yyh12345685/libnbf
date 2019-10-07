#pragma once

#include <unordered_map>
#include <mutex>
#include "common/logger.h"
#include "common/spin_lock.h"

namespace bdf {

class ServerConnect;

class ServerConnectDelayReleaseMgr{
public:
  ServerConnectDelayReleaseMgr() :
    connect_squency_(0), 
    del_time_(0){
  }

  ~ServerConnectDelayReleaseMgr(){
    Release();
  }
  void Release();

  void AddConnect(ServerConnect* connect);
  bool SetRelease(ServerConnect* con);

  void DeleteDalayCon();
protected:
  typedef std::pair<time_t, ServerConnect*> ConnInfo;
  typedef std::unordered_map<int64_t, ConnInfo> DelayDeleteList;
  typedef std::pair<int64_t, ConnInfo> DelayDeletePair;
  typedef std::unordered_map<int64_t, ConnInfo>::iterator DelayDeleteListIt;
private:
  std::atomic<int64_t> connect_squency_;
  std::mutex mutex_lock_;

  //because fd will be reused,so use connect_squency_ as key
  std::unordered_map<int64_t, ServerConnect*> connected_list_;

  //for delay delete
  DelayDeleteList delay_delete_connect_list_;
  std::mutex con_mutex_lock_;

  time_t del_time_;

  LOGGER_CLASS_DECL(logger_);
};

}

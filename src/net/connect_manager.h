#pragma once

#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <mutex>
#include <atomic>
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
  // Undo AddConnect for a connection that never got registered.
  void Remove(ServerConnect* con);
  // True while con is open (added and not yet released). Replies carry the
  // connection as a raw pointer; a closed one is deleted 90s later, so an
  // I/O thread must check this before touching the pointer.
  bool IsLive(ServerConnect* con);

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
  // Open connections, sharded so the per-reply IsLive check on every I/O
  // thread does not serialize on one global lock.
  struct LiveShard {
    std::mutex lock;
    std::unordered_set<ServerConnect*> set;
  };
  static const size_t kLiveShards = 64;
  LiveShard live_[kLiveShards];
  LiveShard& ShardOf(ServerConnect* con) {
    uintptr_t v = reinterpret_cast<uintptr_t>(con);
    return live_[(v >> 4 ^ v >> 12) % kLiveShards];
  }
  void SetLive(ServerConnect* con, bool live);

  //for delay delete
  DelayDeleteList delay_delete_connect_list_;
  std::mutex con_mutex_lock_;

  time_t del_time_;

  LOGGER_CLASS_DECL(logger_);
};

}

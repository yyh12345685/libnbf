#pragma once

#include <unordered_map>
#include "common/logger.h"
#include "common/spin_lock.h"

namespace bdf {

class Connecting;

//为了减少锁的互斥，用10个桶吧，其实更好的办法是实现无锁的map
#define CONNECT_BUCKET 10

//暂时只用于server端接收到的连接管理
class ConnectManager{
public:
  static ConnectManager& Instance() {
    static ConnectManager instance_;
    return instance_;
  }

  bool RegisterConnect(uint64_t desc_id, Connecting* con);
  bool UnRegisterConnect(uint64_t desc_id);
  Connecting* GetConnect(uint64_t desc_id);

private:
  ConnectManager();

  std::vector< std::unordered_map<uint64_t, Connecting*> > id_connect_map_;
  SpinLock locks_[CONNECT_BUCKET];

  LOGGER_CLASS_DECL(logger_);
};

}

#include "connect_manager.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ConnectManager);

ConnectManager::ConnectManager(){
  id_connect_map_.resize(CONNECT_BUCKET);
  //locks_.resize(CONNECT_BUCKET);
}

bool ConnectManager::RegisterConnect(uint64_t desc_id, Connecting* con){
  //for debug
  static time_t now = time(NULL);
  time_t cur_time = time(NULL);

  uint32_t bucket = desc_id % CONNECT_BUCKET;
  locks_[bucket].Lock();
  bool ret = id_connect_map_[bucket].insert(
    std::pair<uint64_t, Connecting*>(desc_id,con)).second;
  if (!ret){
    WARN(logger_, "RegisterConnect failed,desc_id:" << desc_id);
    locks_[bucket].UnLock();
    return false;
  }
  if ((cur_time - now) > 180) {
    //for debug
    INFO(logger_, "bucket id:" << bucket
      << ",connect size:" << id_connect_map_[bucket].size());
    now = cur_time;
  }
  locks_[bucket].UnLock();
  
  return true;
}

bool ConnectManager::UnRegisterConnect(uint64_t desc_id){
  uint32_t bucket = desc_id % CONNECT_BUCKET;
  locks_[bucket].Lock();
  id_connect_map_[bucket].erase(desc_id);
  locks_[bucket].UnLock();
  return true;
}

Connecting* ConnectManager::GetConnect(uint64_t desc_id){
  uint32_t bucket = desc_id % CONNECT_BUCKET;
  locks_[bucket].Lock();
  const auto itr = id_connect_map_[bucket].find(desc_id);
  if (itr != id_connect_map_[bucket].end()){
    locks_[bucket].UnLock();
    return itr->second;
  }
  locks_[bucket].UnLock();
  return nullptr;
}

}

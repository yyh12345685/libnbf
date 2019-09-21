#include <unistd.h>
#include "connect_manager.h"
#include "net/server_connect.h"

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

LOGGER_CLASS_IMPL(logger_, ServerConnectDelayReleaseMgr);

//延迟时间
const static int delay_delete_times = 50;//second
//最小的删除执行间隔
const static int delete_interval_times = 10;//second

void ServerConnectDelayReleaseMgr::Release(){
  if (connected_list_.size() > 0){
    INFO(logger_, "may be memory leak:" << connected_list_.size());
  }

  //sleep(5); for wait connection release
  sleep(5);

  DeleteDalayCon();

  for (const auto& it : connected_list_){
    TRACE(logger_, "connect id:" << it.first << ", addr:" << it.second);
    if (it.second != NULL){
      delete it.second;
    }
  }
  connected_list_.clear();
}

bool ServerConnectDelayReleaseMgr::SetRelease(ServerConnect* con){
  mutex_lock_.lock();
  size_t ret = connected_list_.erase(con->GetConnectId());
  mutex_lock_.unlock();
  TRACE(logger_, "erase id:" << con->GetConnectId() << ", ret:" << ret);
  if (0 == ret){
    WARN(logger_, "connected_list_.size():" << connected_list_.size() 
      << ",fd:" << con->GetFd()<< ",erase failed,may be memory leak,id:" 
      << con->GetConnectId());
    return false;
  }

  bool ret_v = true;
  //add to delay list
  con_mutex_lock_.lock();
  if (!delay_delete_connect_list_.insert(
    DelayDeletePair(con->GetConnectId(), ConnInfo(time(NULL), con))).second){
    WARN(logger_, "insert failed, will be momory leak:" << ",fd:"
      << con->GetFd() << ",key:" << con->GetConnectId());
    ret_v = false;
  }
  con_mutex_lock_.unlock();
  return ret_v;
}

void ServerConnectDelayReleaseMgr::DeleteDalayCon(){
  int cur_time = time(NULL);
  if ((cur_time - del_time_) > delete_interval_times && 
    !delay_delete_connect_list_.empty()){
    INFO(logger_, "delay_delete_connect_list_ size:" << delay_delete_connect_list_.size()
      << ",connected_list_ size:" << connected_list_.size());
    con_mutex_lock_.lock();
    DelayDeleteListIt ls = delay_delete_connect_list_.begin();
    while (ls != delay_delete_connect_list_.end()){
      if ((cur_time - ls->second.first) > delay_delete_times) {
        delete ls->second.second;
        ls = delay_delete_connect_list_.erase(ls);
      } else {
        ls++;
      }
    }
    con_mutex_lock_.unlock();
    del_time_ = cur_time;
  }
}

void ServerConnectDelayReleaseMgr::AddConnect(ServerConnect* connect){
  DeleteDalayCon();

  if (connect_squency_ < 0){
    connect_squency_ = 0;
  }
  connect_squency_++;

  connect->SetConnectId(connect_squency_);
  bool insert_ok = false;
  mutex_lock_.lock();
  insert_ok = connected_list_.insert(
    std::pair<int64_t, ServerConnect*>(connect->GetConnectId(), connect)).second;
  mutex_lock_.unlock();

  if (!insert_ok){
    WARN(logger_, "insert faild,may be memory leak,id:" << connect->GetConnectId());
  }
}

}


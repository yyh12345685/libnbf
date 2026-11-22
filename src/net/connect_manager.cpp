#include "connect_manager.h"
#include "net/server_connect.h"
#include "monitor/mem_profile.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ServerConnectDelayReleaseMgr);

static const std::chrono::seconds delay_delete_time(90);

void ServerConnectDelayReleaseMgr::Release(){
  for (const auto& it : connected_list_){
    SetLive(it.second, false);
    BDF_DELETE(it.second);
  }
  connected_list_.clear();
  for (const auto& entry : delay_delete_connect_list_) {
    BDF_DELETE(entry.second);
  }
  delay_delete_connect_list_.clear();
}

bool ServerConnectDelayReleaseMgr::SetRelease(ServerConnect* con){
  mutex_lock_.lock();
  size_t ret = connected_list_.erase(con->GetConnectId());
  mutex_lock_.unlock();
  SetLive(con, false);
  TRACE(logger_, "erase id:" << con->GetConnectId() << ", ret:" << ret);
  if (0 == ret){
    WARN(logger_, "fd:" << con->GetFd()<< ",erase failed,may be memory leak,id:"
      << con->GetConnectId());
    return false;
  }

  std::lock_guard<std::mutex> guard(con_mutex_lock_);
  delay_delete_connect_list_.emplace_back(Clock::now(), con);
  return true;
}

void ServerConnectDelayReleaseMgr::Remove(ServerConnect* con){
  {
    std::lock_guard<std::mutex> guard(mutex_lock_);
    connected_list_.erase(con->GetConnectId());
  }
  SetLive(con, false);
}

void ServerConnectDelayReleaseMgr::SetLive(ServerConnect* con, bool live){
  LiveShard& shard = ShardOf(con);
  std::lock_guard<std::mutex> guard(shard.lock);
  if (live) shard.set.insert(con);
  else shard.set.erase(con);
}

bool ServerConnectDelayReleaseMgr::IsLive(ServerConnect* con){
  LiveShard& shard = ShardOf(con);
  std::lock_guard<std::mutex> guard(shard.lock);
  return shard.set.count(con) > 0;
}

void ServerConnectDelayReleaseMgr::DeleteDelayedConnections(){
  const auto now = Clock::now();
  std::unique_lock<std::mutex> guard(con_mutex_lock_);
  while (!delay_delete_connect_list_.empty()
         && now - delay_delete_connect_list_.front().first >= delay_delete_time) {
    ServerConnect* connection = delay_delete_connect_list_.front().second;
    delay_delete_connect_list_.pop_front();
    guard.unlock();
    BDF_DELETE(connection);
    guard.lock();
  }
}

void ServerConnectDelayReleaseMgr::AddConnect(ServerConnect* connect){
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
  if (insert_ok) SetLive(connect, true);

  if (!insert_ok){
    WARN(logger_, "insert faild,may be memory leak,id:" << connect->GetConnectId());
  }
}

}


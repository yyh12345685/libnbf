
#pragma once

#include "event/event_data.h"
#include "event/event_notifier.h"
#include <unordered_set>
#include <deque>

namespace bdf{

//template <typename LockPolicy=NoLock>
class EventDriver{
public:
  EventDriver();
  virtual ~EventDriver();
  int Init();
  int Run();
  int Stop();
  int ShutDown();
  int Add(int fd, EventFunctionBase *ezfd,bool lock = false);
  int Del(int fd);
  int Modr(int fd, bool set);
  int Modw(int fd, bool set);
  int Poll(int timeout/*msecs*/);

  //if used for agent_master
  std::unordered_set<int>& GetListenedListFd(){
    return listened_list_fd_; 
  }

  void AddListenedListFd(int fd){ 
    listened_list_fd_.insert(fd); 
  }

  bool GetRun(){ return run_; }
private:

  int Mod(int fd);
  
  bool run_;
  bool inloop_;
  int epfd_;
  int maxfd_;
  
  uint32_t event_in_;  
  uint32_t event_out_;

  LOGGER_CLASS_DECL(logger_);

  std::unordered_set<int> listened_list_fd_;

  EventData event_data_;

  EventNotifier event_notifier_;
};

}

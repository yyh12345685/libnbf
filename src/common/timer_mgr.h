
#pragma once

#include <map>
#include <tr1/unordered_map>
#include "common/logger.h"

namespace bdf {

class TimerMgrBase{
public:
  virtual void OnTimer(void* timer_data,uint64_t time_id) = 0;
  virtual ~TimerMgrBase() { }
  void* timer_data_;
};

typedef uint64_t timer_id;

class TimerMgr {
public:
  typedef std::pair<uint64_t, TimerMgrBase*> TimerInfo;
  typedef std::multimap<uint64_t, TimerInfo> TimerMap;// time -> (id, TimerBase *)
  typedef TimerMap::iterator TimerItem;
  typedef std::tr1::unordered_map<uint64_t/*id*/, TimerItem> TimerIdMap; // id -> iterator of TimerMap
  typedef TimerIdMap::iterator IdIter; // id -> iterator of id_map

  TimerMgr(){
	}

  ~TimerMgr(){
    TimerItem iter = timer_map_.begin();
    for (; iter != timer_map_.end(); iter++){
      TimerInfo info = iter->second;
      timer_id id = info.first;
      TimerMgrBase *timer = info.second;
      timer->OnTimer(timer->timer_data_,id);
    }
    timer_map_.clear();
    id_map_.clear();
  }
    
  uint64_t AddTimer(TimerMgrBase *timer, uint64_t after);
  int DelTimer(timer_id id);
  int RunTimer();
    
  void AddTimerInThread(timer_id id, TimerMgrBase *timer, uint64_t when);
  void DelTimerInThread(timer_id id);

  uint64_t GetTimeMapSize(){ return timer_map_.size(); }
private:
  uint64_t GetTimerId();

  TimerIdMap id_map_;
  TimerMap timer_map_;
  //uint64_t timer_id_;
	
	LOGGER_CLASS_DECL(logger_);
};

}

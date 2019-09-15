
#include "common/timer_mgr.h"
#include "common/time.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, TimerMgr)

uint64_t TimerMgr::GetTimerId(){
  volatile static uint64_t tmid = 0;
  uint64_t id = __sync_add_and_fetch(&tmid, 1);
  if (id > 0) {
    return id;
  } else {
    //start with 1
    tmid = 1;
    return 1;
  }
}

void TimerMgr::AddTimerInThread(timer_id id, TimerMgrBase *timer, uint64_t when){
  TimerItem iter = timer_map_.insert(std::make_pair(when, std::make_pair(id, timer)));
  id_map_.insert(std::make_pair(id, iter));
  //assert(id_map_.insert(std::make_pair(id, iter)).second);
}

void TimerMgr::DelTimerInThread(timer_id id){
  IdIter iter = id_map_.find(id);
  if (iter != id_map_.end()){
    TimerItem titer = iter->second;
    id_map_.erase(iter);
    timer_map_.erase(titer);
  }
}

timer_id TimerMgr::AddTimer(TimerMgrBase *timer, uint64_t after){
  //assert(timer);
  //assert(after >= 0);
	if(NULL == timer || after < 0){
		return -1;
	}

  uint64_t id = GetTimerId();
  uint64_t when = Time::GetCurrentClockTime() + after;
 
  /*if (NULL== poll || poll->InThread()){
    AddTimerInThread(id, timer, when);
  }else{
    EvAddTimer* tm = new EvAddTimer(id, timer, when);
    poll->RunTaskAsync(tm);
  }*/
  AddTimerInThread(id, timer, when);
  
  return id;
}

int TimerMgr::DelTimer(timer_id id){
  DelTimerInThread(id);

  /*if (poll == NULL || poll->InThread()){
    DelTimerInThread(id);
  }else{
    EvDelTimer* t_del = new EvDelTimer(id);
    poll->RunTaskAsync(t_del);
  }*/
  
  return 0;
}

int TimerMgr::RunTimer(){
  uint64_t now = Time::GetCurrentClockTime();
  while (!timer_map_.empty()){
    TimerItem iter = timer_map_.begin();
    if (iter->first >= now) {
      // >= to avoid infinite loop if user call "AddTimer(timer, 0)"
      break;
    }
		//TRACE(logger_,"iter->first:"<<iter->first<<",now:"<<now);

    TimerInfo info = iter->second;
    timer_map_.erase(iter);
    timer_id id = info.first;
    id_map_.erase(id);
    TimerMgrBase *timer = info.second;
    timer->OnTimer(timer->timer_data_, id);
  }
  return 0;
}

}

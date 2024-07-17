
#include "coroutine/coroutine_manager.h"

namespace bdf{

  void CoroutineManager::Init(
    CoroutineServiceHandler* service_handle, 
    TimerMgr* time_mgr) {
  Instance().time_mgr_ = time_mgr;
  Instance().service_handle_ = service_handle;
  Instance().scheduler_ = new CoroutineSchedule;
}

}
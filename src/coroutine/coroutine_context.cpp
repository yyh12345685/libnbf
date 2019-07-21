
#include "coroutine/coroutine_context.h"

namespace bdf{

void CoroutineContext::Init(CoroutineServiceHandler* service_handle,Timer* timer) {
  Instance().timer_ = timer;
  Instance().service_handle_ = service_handle;
  Instance().scheduler_ = new CoroutineSchedule;
  Instance().cur_coroutine_id_ = -1;
}

}
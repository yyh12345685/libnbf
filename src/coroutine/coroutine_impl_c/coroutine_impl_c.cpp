
#include <string.h>
#include <assert.h>
#include "coroutine_impl_c.h"
#include "coroutine_actor_c.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coro_context.h"
#include "coro_context_c.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineImplc);

//from https://github.com/cloudwu/coroutine

//上限暂时设置为10万
#define MAX_CORO_IDS 100000

CoroContextc* CoNew(
  CoroContextList* corotine_ls, CoroutineFunc func, void* ud, int stack_size) {
  CoroContextc* coctx = new CoroContextc;
  coctx->init_func = func;
  coctx->ud = ud;
  //coctx->ls = corotine_ls;
  //coctx->ctx_cap = 0;
  coctx->size = stack_size;
  coctx->status = Coroutine::kCoroutineReady;
  coctx->stack = (char*)malloc(stack_size); 
  coctx->ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
  return coctx;
}

void CoDelete(CoroContextc* coctx) {
  if (coctx->actor != nullptr) {
    delete coctx->actor;
  }

  if (coctx->ctx != nullptr) {
    free(coctx->ctx);
  }

  if (coctx->stack != nullptr) {
    free(coctx->stack);
  }
  
  delete(coctx);
}

bool CoroutineImplc::CoroutineInit(
  CoroutineFunc func, void* data, CoroRelease* release,
  std::unordered_set<CoroContext*>& free_list, int coroutine_size, int stack_size) {
  coro_ls_ = new CoroContextList;
  //coro_ls_->nco = 0;
  coro_ls_->cap = coroutine_size;
  coro_ls_->running = nullptr;
  if (stack_size <= 0) {
    coro_ls_->stack_size = DEFAULT_STACK_SIZE;
  } else {
    coro_ls_->stack_size = stack_size;
  }
  //coro_ls_->coctxs = (CoroutineActor**)malloc(sizeof(CoroutineActor*) * coro_ls_->cap);
  //memset(coro_ls_->coctxs, 0, sizeof(CoroutineActor*) * coro_ls_->cap);
  //coro_ls_->coctxs.resize(coro_ls_->cap);

  TRACE(logger_, "ThreadId:" << ThreadId::Get() << ",will create coro size:" << coro_ls_->cap);
  for (int idx = 0; idx < coro_ls_->cap; idx++) {
    CoroContext* coro = CoroutineNew(func, data, release, stack_size);
    free_list.insert(coro);
  }

  return true;
}

void CoroutineImplc::Release() {
  assert(coro_ls_ != nullptr);
  for (CoroContext* coro : coro_ls_->coctxs) {
    CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro);
    if (coctx) {
      CoDelete(coctx);
    }
  }
  delete(coro_ls_);
}

int CoroutineImplc::CoroutineSize() {
  assert(coro_ls_ != nullptr);
  return coro_ls_->cap;
}

CoroContext* CoroutineImplc::CoroutineNew(
  CoroutineFunc func, void *ud, CoroRelease* release, int stack_size) {
  assert(coro_ls_ != nullptr);
  if (coro_ls_->cap >= MAX_CORO_IDS) {
    WARN(logger_, "coroutine is limited:" << coro_ls_->cap);
    return nullptr;
  }
  if (stack_size <= 0) {
    stack_size = DEFAULT_STACK_SIZE;
  } 

  CoroContextc* coctx = CoNew(coro_ls_, func, ud, stack_size);
  coro_ls_->coctxs.insert(coctx);
  coctx->actor = new CoroutineActorc();
  coctx->release = release;
  INFO(logger_, "ThreadId:" << ThreadId::Get() 
    << ",create coroutine ptr:" << coctx << ",idx:" << coro_ls_->coctxs.size());
  return coctx;
}

static void MainFunc(uint32_t low32, uint32_t hi32) {
  uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
  CoroContextList* corotine_ls = (CoroContextList*)ptr;
  CoroContextc* coctx = dynamic_cast<CoroContextc*>(corotine_ls->running);
  if (nullptr != coctx->start_func) {
    coctx->start_func(coctx->ud);
  } else {
    coctx->init_func(coctx->ud);
  }
  
  if (coctx->release != nullptr) {
    coctx->release->ReleaseResource(coctx);
  }
  corotine_ls->coctxs.erase(coctx);
  CoDelete(coctx);
  
  --corotine_ls->cap;
  corotine_ls->running = nullptr;
}

//启动协程
void CoroutineImplc::CoroutineStart(CoroContext* coro, CoroutineFunc func, void* data) {
  assert(coro_ls_ != nullptr);
  if (coro_ls_->running != nullptr){
    WARN(logger_, "CoroutineStart error running:" << coro_ls_->running);
    return ;
  }

  CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro);
  if (nullptr == coctx)
    return ;
  TRACE(logger_, "CoroutineStart coro:" << coctx << ",func:" << func);
  int status = coctx->status;
  coctx->start_func = func; // 如果传入的func为空后者不穿，则使用初始化时传入的协程函数
  if (data != nullptr) {
    coctx->ud = data; // 支持启动的时候再传入协程函数参数
  }
  if (status == kCoroutineReady) {
    getcontext(coctx->ctx);
    coctx->ctx->uc_stack.ss_sp = coctx->stack;
    coctx->ctx->uc_stack.ss_size = coctx->size;
    coctx->ctx->uc_link = &coro_ls_->main_ctx;
    coro_ls_->running = coro;
    coctx->status = kCoroutineRunning;
    uintptr_t ptr = (uintptr_t)coro_ls_;
    makecontext(coctx->ctx, (void(*)(void))MainFunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
    swapcontext(&coro_ls_->main_ctx, coctx->ctx);
  } else {
    WARN(logger_, "CoroutineStart unkown status:" << status);
  }
}

bool CoroutineImplc::CoroutineResume(CoroContext* coro) {
  assert(coro_ls_ != nullptr);
  if (coro_ls_->running != nullptr){
    WARN(logger_, "CoroutineResume error running:" << coro_ls_->running);
    return false;
  }

  CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro);
  if (nullptr == coctx)
    return false;
  int status = coctx->status;
  //想看详细日志可以打开
  //TRACE(logger_, "CoroutineResume coro:" << coro << ",status:" << status);
  switch (status) {
  case kCoroutineReady: {
    getcontext(coctx->ctx);
    coctx->ctx->uc_stack.ss_sp = coctx->stack;
    coctx->ctx->uc_stack.ss_size = coctx->size;
    coctx->ctx->uc_link = &coro_ls_->main_ctx;
    coro_ls_->running = coro;
    coctx->status = kCoroutineRunning;
    uintptr_t ptr = (uintptr_t)coro_ls_;
    makecontext(coctx->ctx, (void(*)(void))MainFunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
    swapcontext(&coro_ls_->main_ctx, coctx->ctx); // 对称协程，从线程对应的主协程切换到coro来运行
    break; 
  }
  case kCoroutineSuspend:
    //memcpy(coro_ls_->stack + STACK_SIZE - coctx->size, coctx->stack, coctx->size);
    coro_ls_->running = coro;
    coctx->status = kCoroutineRunning;
    swapcontext(&coro_ls_->main_ctx, coctx->ctx); // 对称协程，从线程对应的主协程切换到coro来运行
    break; 
  default:
    WARN(logger_, "ThreadId:" << ThreadId::Get() << ",unkown status:" << status << ",coro:" << coro);
    return false;
    //break;
  }
  //想看详细日志可以打开
  //TRACE(logger_, "changed status:" << status << ",running id:" << coro_ls_->running);
  return true;
}

/*void CoroutineImplc::SaveStack(CoroContextc* coctx, char *top) {
  char dummy = 0;
  assert(top - &dummy <= STACK_SIZE);
  if (coctx->ctx_cap < top - &dummy) {
    INFO(logger_, "stack size is:" << coctx->size << ",cap:" 
      << coctx->ctx_cap << ",will change to:" << (top - &dummy) << ",coro:" << coctx);
    if (coctx->stack != nullptr) {
      free(coctx->stack);
    }
    coctx->ctx_cap = top - &dummy;
    coctx->stack = (char*)malloc(coctx->ctx_cap);
  }
  coctx->size = top - &dummy;
  memcpy(coctx->stack, &dummy, coctx->size);
}*/

void CoroutineImplc::CoroutineYield() {
  assert(coro_ls_ != nullptr);
  // 框架要求该函数运行在协程中
  if (coro_ls_->running == nullptr) {
    WARN(logger_, "CoroutineYield error running:" << coro_ls_->running);
    return;
  }

  CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro_ls_->running);
  if (nullptr == coctx)
    return;
  /*if (coro_ls_->stack >= (char *)&coctx){
    WARN(logger_, "error stack:" << coro_ls_->stack);
    return;
  }*/
  //SaveStack(coctx, coro_ls_->stack + STACK_SIZE);
  char dummy = 0;
  if (abs(coctx->stack - &dummy) > coctx->size) {
    WARN(logger_, "ThreadId:" << ThreadId::Get() << ",coroutine ptr:" << coctx 
      << ",stack size is :" << abs(coctx->stack - &dummy) << " more than:" << coctx->size);
  }
  coctx->status = kCoroutineSuspend;
  coro_ls_->running = nullptr;
  // 对称协程，切换到线程对应的主协程来运行
  swapcontext(coctx->ctx, &coro_ls_->main_ctx);
}

int CoroutineImplc::CoroutineStatus(CoroContext* coro) {
  assert(coro_ls_ != nullptr);

  CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro);
  if (nullptr == coctx)
    return kCoroutineInvalid;

  if (coctx == nullptr) {
    return kCoroutineInvalid;
  }
  return coctx->status;
}

CoroContext* CoroutineImplc::GetCurrentCoroutine() {
  assert(coro_ls_ != nullptr);
  return coro_ls_->running;
}

}

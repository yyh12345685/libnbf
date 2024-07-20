
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
  CoroContextList* corotine_ls, CoroutineFunc func, void* ud) {
  CoroContextc* coctx = new CoroContextc;
  coctx->func = func;
  coctx->ud = ud;
  //coctx->ls = corotine_ls;
  coctx->ctx_cap = 0;
  coctx->size = 0;
  coctx->status = Coroutine::kCoroutineReady;
  coctx->stack = nullptr; 
  return coctx;
}

void CoDelete(CoroContextc* coctx) {
  if (coctx->actor != nullptr) {
    delete coctx->actor;
  }
  free(coctx->stack);
  delete(coctx);
}

bool CoroutineImplc::CoroutineInit(
  CoroutineFunc func, void* data, 
  std::queue<CoroContext*>& free_list, int coroutine_size) {
  coro_ls_ = new CoroContextList;
  coro_ls_->nco = 0;
  coro_ls_->cap = coroutine_size;
  coro_ls_->running = nullptr;
  //coro_ls_->coctxs = (CoroutineActor**)malloc(sizeof(CoroutineActor*) * coro_ls_->cap);
  //memset(coro_ls_->coctxs, 0, sizeof(CoroutineActor*) * coro_ls_->cap);
  //coro_ls_->coctxs.resize(coro_ls_->cap);

  TRACE(logger_, "ThreadId:" << ThreadId::Get() << ",will create coro size:" << coro_ls_->cap);
  for (int idx = 0; idx < coro_ls_->cap; idx++) {
    InitNewCoroutine(func, data, free_list);
  }

  return true;
}

int CoroutineImplc::InitNewCoroutine(
  CoroutineFunc func, void* data, std::queue<CoroContext*>& free_list) {
  CoroContext* coro = CoroutineNew(func, data);
  free_list.push(coro);

  //all_coro_list_.emplace_back(coroutine_id);
  //available_coro_list_.insert(coroutine_id);
  //if (coroutine_id > max_coro_id_) {
  //  max_coro_id_ = coroutine_id;
  //}

  return 0;
}

void CoroutineImplc::Release() {
  assert(coro_ls_ != nullptr);
  /*for (int idx = 0; idx < coro_ls_->cap; idx++) {
    CoroContextc* coctx = coro_ls_->coctxs[idx];
    if (coctx) {
      CoDelete(coctx);
    }
  }*/
  for (CoroContext* coro : coro_ls_->coctxs) {
    CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro);
    if (coctx) {
      CoDelete(coctx);
    }
  }
  //free(corotine->coctxs);
  //corotine->coctxs = nullptr;
  delete(coro_ls_);
}

int CoroutineImplc::CoroutineSize() {
  assert(coro_ls_ != nullptr);
  return coro_ls_->cap;
}

CoroContext* CoroutineImplc::CoroutineNew(CoroutineFunc func, void *ud) {
  assert(coro_ls_ != nullptr);
  if (coro_ls_->nco >= coro_ls_->cap) {
    WARN(logger_, "coroutine is limited:" << coro_ls_->cap);
    return nullptr;
  }
  CoroContextc* coctx = CoNew(coro_ls_, func, ud);
  coro_ls_->coctxs.insert(coctx);
  //coro_ls_->cap++;
  coro_ls_->nco++;
  coctx->actor = new CoroutineActorc();
  TRACE(logger_, "ThreadId:" << ThreadId::Get() << "create coroutine ptr:" << coctx << ",idx:" << coro_ls_->nco);
  return coctx;

  /*if (coro_ls_->nco >= coro_ls_->cap) {
    int id = coro_ls_->cap;
    //corotine->coctxs = 
    //  (CoroutineActor**)realloc(corotine->coctxs, corotine->cap * 2 * sizeof(CoroutineActor*));
    //memset(corotine->coctxs + corotine->cap, 0, sizeof(CoroutineActor *) * corotine->cap);
    //corotine->coctxs[corotine->cap] = coctx;
    //corotine->cap *= 2;
    coro_ls_->coctxs.emplace_back(coctx);
    coro_ls_->cap++;
    ++coro_ls_->nco;
    TRACE(logger_, "create coroutine_id_:" << id <<",ptr:"<< coctx);
    return id;
  } else {
    for (int idx = 0; idx < coro_ls_->cap; idx++) {
      int id = (idx + coro_ls_->nco) % coro_ls_->cap;
      if (coro_ls_->coctxs[id] == nullptr) {
        coro_ls_->coctxs[id] = coctx;
        ++coro_ls_->nco;
        TRACE(logger_, "create coroutine_id_:" << id << ",ptr:" << coctx);
        return id;
      }
    }
  }
  return -1;*/
}

static void MainFunc(uint32_t low32, uint32_t hi32) {
  uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
  CoroContextList*corotine_ls = (CoroContextList*)ptr;
  CoroContextc* coctx = dynamic_cast<CoroContextc*>(corotine_ls->running);
  coctx->func(coctx->ud);
  CoDelete(coctx);
  // corotine_ls->coctxs[id] = nullptr; TODO 刪除本协程的数据
  // corotine_ls->coctxs.erase(coctx);
  --corotine_ls->nco;
  corotine_ls->running = nullptr;
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
  //TRACE(logger_, "status:" << status << ",running id:" << coro_ls_->running);
  switch (status) {
  case kCoroutineReady: {
    getcontext(&coctx->ctx);
    coctx->ctx.uc_stack.ss_sp = coro_ls_->stack;
    coctx->ctx.uc_stack.ss_size = STACK_SIZE;
    coctx->ctx.uc_link = &coro_ls_->main_ctx;
    coro_ls_->running = coro;
    coctx->status = kCoroutineRunning;
    uintptr_t ptr = (uintptr_t)coro_ls_;
    makecontext(&coctx->ctx, (void(*)(void))MainFunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
    swapcontext(&coro_ls_->main_ctx, &coctx->ctx);
    break; 
  }
  case kCoroutineSuspend:
    memcpy(coro_ls_->stack + STACK_SIZE - coctx->size, coctx->stack, coctx->size);
    coro_ls_->running = coro;
    coctx->status = kCoroutineRunning;
    swapcontext(&coro_ls_->main_ctx, &coctx->ctx);
    break; 
  default:
    WARN(logger_, "unkown status:" << status);
    return false;
    //break;
  }
  //想看详细日志可以打开
  //TRACE(logger_, "changed status:" << status << ",running id:" << coro_ls_->running);
  return true;
}

void CoroutineImplc::SaveStack(CoroContextc* coctx, char *top){
  char dummy = 0;
  if (top - &dummy >= STACK_SIZE){
    ERROR(logger_, "stack is more than:" << STACK_SIZE);
    return;
  }
  if (coctx->ctx_cap < top - &dummy) {
    free(coctx->stack);
    coctx->ctx_cap = top - &dummy;
    coctx->stack = (char*)malloc(coctx->ctx_cap);
  }
  coctx->size = top - &dummy;
  memcpy(coctx->stack, &dummy, coctx->size);
}

void CoroutineImplc::CoroutineYield() {
  assert(coro_ls_ != nullptr);
  // TODO 需要处理在协程中和不在协程中的情况[如在线程中时情况存在吗？]
  if (coro_ls_->running == nullptr) {
    WARN(logger_, "CoroutineYield error running:" << coro_ls_->running);
    return;
  }

  CoroContextc* coctx = dynamic_cast<CoroContextc*>(coro_ls_->running);
  if (nullptr == coctx)
    return;
  if (coro_ls_->stack >= (char *)&coctx){
    WARN(logger_, "error stack:" << coro_ls_->stack);
    return;
  }
  SaveStack(coctx, coro_ls_->stack + STACK_SIZE);
  coctx->status = kCoroutineSuspend;
  coro_ls_->running = nullptr;
  swapcontext(&coctx->ctx, &coro_ls_->main_ctx);
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

int CoroutineImplc::CoroutineRunning() {
  return coro_ls_->running != nullptr;
}

}

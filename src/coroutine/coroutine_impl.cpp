
#include <string.h>
#include "coroutine/coroutine_impl.h"
#include "coroutine/coroutine_actor.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineImpl);

//from https://github.com/cloudwu/coroutine

CoroutineActor* CoNew(
  CoroSchedule *corotine, CoroutineFunc func, void *ud) {
  CoroutineActor* coctx = new CoroutineActor;
  coctx->func = func;
  coctx->ud = ud;
  coctx->corotine = corotine;
  coctx->ctx_cap = 0;
  coctx->size = 0;
  coctx->status = CoroutineImpl::kCoroutineReady;
  coctx->stack = nullptr;
  return coctx;
}

void CoDelete(CoroutineActor*coctx) {
  free(coctx->stack);
  delete(coctx);
}

CoroSchedule* CoroutineImpl::CoroutineInit(int coroutine_size) {
  CoroSchedule* corotine = new CoroSchedule;
  corotine->nco = 0;
  corotine->cap = coroutine_size;
  corotine->running = -1;
  //corotine->coctxs = (CoroutineActor**)malloc(sizeof(CoroutineActor*) * corotine->cap);
  //memset(corotine->coctxs, 0, sizeof(CoroutineActor*) * corotine->cap);
  corotine->coctxs.resize(corotine->cap);
  return corotine;
}

void CoroutineImpl::CoroutineClose(CoroSchedule *corotine) {
  for (int idx = 0; idx < corotine->cap; idx++) {
    CoroutineActor * coctx = corotine->coctxs[idx];
    if (coctx) {
      CoDelete(coctx);
    }
  }
  //free(corotine->coctxs);
  //corotine->coctxs = nullptr;
  delete(corotine);
}

int CoroutineImpl::CoroutineNew(CoroSchedule *corotine, CoroutineFunc func, void *ud) {
  CoroutineActor *coctx = CoNew(corotine, func, ud);
  if (corotine->nco >= corotine->cap) {
    int id = corotine->cap;
    //corotine->coctxs = 
    //  (CoroutineActor**)realloc(corotine->coctxs, corotine->cap * 2 * sizeof(CoroutineActor*));
    //memset(corotine->coctxs + corotine->cap, 0, sizeof(CoroutineActor *) * corotine->cap);
    //corotine->coctxs[corotine->cap] = coctx;
    //corotine->cap *= 2;
    corotine->coctxs.emplace_back(coctx);
    corotine->cap++;
    ++corotine->nco;
    TRACE(logger_, "create coroutine_id_:" << id <<",ptr:"<< coctx);
    return id;
  } else {
    for (int idx = 0; idx < corotine->cap; idx++) {
      int id = (idx + corotine->nco) % corotine->cap;
      if (corotine->coctxs[id] == nullptr) {
        corotine->coctxs[id] = coctx;
        ++corotine->nco;
        TRACE(logger_, "create coroutine_id_:" << id << ",ptr:" << coctx);
        return id;
      }
    }
  }
  return -1;
}

static void MainFunc(uint32_t low32, uint32_t hi32) {
  uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
  CoroSchedule *corotine = (CoroSchedule *)ptr;
  int id = corotine->running;
  CoroutineActor *coctx = corotine->coctxs[id];
  coctx->func(/*corotine, */coctx->ud);
  CoDelete(coctx);
  corotine->coctxs[id] = nullptr;
  --corotine->nco;
  corotine->running = -1;
}

void CoroutineImpl::CoroutineResume(CoroSchedule* corotine, int id) {
  if (corotine->running != -1){
    WARN(logger_, "error running:" << corotine->running);
    return;
  }
  if (id < 0 || id >= corotine->cap){
    WARN(logger_, "error id:" << id<<",cap:"<< corotine->cap);
    return;
  }

  CoroutineActor *coctx = corotine->coctxs[id];
  if (nullptr == coctx)
    return;
  int status = coctx->status;
  //想看详细日志可以打开
  //TRACE(logger_, "status:" << status << ",running id:" << corotine->running);
  switch (status) {
  case kCoroutineReady: {
    getcontext(&coctx->ctx);
    coctx->ctx.uc_stack.ss_sp = corotine->stack;
    coctx->ctx.uc_stack.ss_size = STACK_SIZE;
    coctx->ctx.uc_link = &corotine->main_ctx;
    corotine->running = id;
    coctx->status = kCoroutineRunning;
    uintptr_t ptr = (uintptr_t)corotine;
    makecontext(&coctx->ctx, (void(*)(void))MainFunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
    swapcontext(&corotine->main_ctx, &coctx->ctx);
    break; 
  }
  case kCoroutineSuspend:
    memcpy(corotine->stack + STACK_SIZE - coctx->size, coctx->stack, coctx->size);
    corotine->running = id;
    coctx->status = kCoroutineRunning;
    swapcontext(&corotine->main_ctx, &coctx->ctx);
    break; 
  default:
    WARN(logger_, "unkown status:" << status);
    break;
  }
  //想看详细日志可以打开
  //TRACE(logger_, "changed status:" << status << ",running id:" << corotine->running);
}

void CoroutineImpl::SaveStack(CoroutineActor* coctx, char *top){
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

void CoroutineImpl::CoroutineYield(CoroSchedule * corotine) {
  int id = corotine->running;
  if (id <0){
    WARN(logger_, "error id:" << id);
    return;
  }
  CoroutineActor * coctx = corotine->coctxs[id];
  if (corotine->stack >= (char *)&coctx){
    WARN(logger_, "error stack:" << corotine->stack);
    return;
  }
  SaveStack(coctx, corotine->stack + STACK_SIZE);
  coctx->status = kCoroutineSuspend;
  corotine->running = -1;
  swapcontext(&coctx->ctx, &corotine->main_ctx);
}

CoroutineActor* CoroutineImpl::GetCoroutineCtx(CoroSchedule * corotine,int id){
  if (id < 0 || id >= corotine->cap) {
    WARN(logger_, "error id:" << id);
    return nullptr;
  }
  return corotine->coctxs[id];
}

int CoroutineImpl::CoroutineStatus(CoroSchedule* corotine, int id) {
  if (id<0 ||id >= corotine->cap){
    WARN(logger_,"error id:"<<id);
    return kCoroutineInvalid;
  }

  if (corotine->coctxs[id] == nullptr) {
    return kCoroutineInvalid;
  }
  return corotine->coctxs[id]->status;
}

int CoroutineImpl::CoroutineRunning(CoroSchedule * corotine) {
  return corotine->running;
}

}

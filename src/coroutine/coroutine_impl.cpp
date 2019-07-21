
#include <string.h>
#include "coroutine/coroutine_impl.h"
#include "coroutine/coroutine_actor.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineImpl);

CoroutineActor* CoNew(
  CoroSchedule *corotine, CoroutineFunc func, void *ud) {
  CoroutineActor* coctx = (CoroutineActor*)malloc(sizeof(*coctx));
  coctx->func = func;
  coctx->ud = ud;
  coctx->corotine = corotine;
  coctx->cap = 0;
  coctx->size = 0;
  coctx->status = CoroutineImpl::kCoroutineReady;
  coctx->stack = nullptr;
  return coctx;
}

void CoDelete(CoroutineActor*coctx) {
  free(coctx->stack);
  free(coctx);
}

CoroSchedule* CoroutineImpl::CoroutineInit() {
  CoroSchedule* corotine = new CoroSchedule;
  //Coroutine *corotine = (Coroutine*)malloc(sizeof(*corotine));
  corotine->nco = 0;
  corotine->cap = DEFAULT_COROUTINE;
  corotine->running = -1;
  corotine->coctx = (CoroutineActor**)malloc(sizeof(CoroutineActor*) * corotine->cap);
  memset(corotine->coctx, 0, sizeof(CoroutineActor*) * corotine->cap);
  return corotine;
}

void CoroutineImpl::CoroutineClose(CoroSchedule *corotine) {
  for (int i = 0; i < corotine->cap; i++) {
    CoroutineActor * coctx = corotine->coctx[i];
    if (coctx) {
      CoDelete(coctx);
    }
  }
  free(corotine->coctx);
  corotine->coctx = nullptr;
  delete(corotine);
}

int CoroutineImpl::CoroutineNew(CoroSchedule *corotine, CoroutineFunc func, void *ud) {
  CoroutineActor *coctx = CoNew(corotine, func, ud);
  if (corotine->nco >= corotine->cap) {
    int id = corotine->cap;
    corotine->coctx = (CoroutineActor**)realloc(corotine->coctx, corotine->cap * 2 * sizeof(CoroutineActor*));
    memset(corotine->coctx + corotine->cap, 0, sizeof(CoroutineActor *) * corotine->cap);
    corotine->coctx[corotine->cap] = coctx;
    corotine->cap *= 2;
    ++corotine->nco;
    TRACE(logger_, "create coroutine_id_:" << id <<",ptr:"<< coctx);
    return id;
  } else {
    for (int idx = 0; idx < corotine->cap; idx++) {
      int id = (idx + corotine->nco) % corotine->cap;
      if (corotine->coctx[id] == nullptr) {
        corotine->coctx[id] = coctx;
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
  CoroutineActor *coctx = corotine->coctx[id];
  coctx->func(/*corotine, */coctx->ud);
  CoDelete(coctx);
  corotine->coctx[id] = nullptr;
  --corotine->nco;
  corotine->running = -1;
}

void CoroutineImpl::CoroutineResume(CoroSchedule* corotine, int id) {
  if (corotine->running != -1){
    WARN(logger_, "error running:" << corotine->running);
    return;
  }
  if (id < 0 || id >= corotine->cap){
    WARN(logger_, "error id" << id<<",cap:"<< corotine->cap);
    return;
  }

  CoroutineActor *coctx = corotine->coctx[id];
  if (coctx == nullptr)
    return;
  int status = coctx->status;
  TRACE(logger_, "status:" << status << ",running id:" << corotine->running);
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
  TRACE(logger_, "changed status:" << status << ",running id:" << corotine->running);
}

void CoroutineImpl::SaveStack(CoroutineActor* coctx, char *top){
  char dummy = 0;
  if (top - &dummy >= STACK_SIZE){
    WARN(logger_, "stack is more than:" << STACK_SIZE);
    return;
  }
  if (coctx->cap < top - &dummy) {
    free(coctx->stack);
    coctx->cap = top - &dummy;
    coctx->stack = (char*)malloc(coctx->cap);
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
  CoroutineActor * coctx = corotine->coctx[id];
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
  return corotine->coctx[id];
}

int CoroutineImpl::CoroutineStatus(CoroSchedule* corotine, int id) {
  if (id<0 ||id >= corotine->cap){
    WARN(logger_,"error id:"<<id);
    return kCoroutineInvalid;
  }

  if (corotine->coctx[id] == nullptr) {
    return kCoroutineInvalid;
  }
  return corotine->coctx[id]->status;
}

int CoroutineImpl::CoroutineRunning(CoroSchedule * corotine) {
  return corotine->running;
}

}

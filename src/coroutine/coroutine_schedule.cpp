
#include <string.h>
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_actor.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineSchedule);

CoroContext* CoNew(
  Coroutine *corotine, CoroutineFunc func, void *ud) {
  CoroContext* coctx = (CoroContext*)malloc(sizeof(*coctx));
  coctx->func = func;
  coctx->ud = ud;
  coctx->corotine = corotine;
  coctx->cap = 0;
  coctx->size = 0;
  coctx->status = CoroutineSchedule::kCoroutineReady;
  coctx->stack = NULL;
  return coctx;
}

void CoDelete(CoroContext*coctx) {
  free(coctx->stack);
  free(coctx);
}

CoroutineActor* CoroutineSchedule::CoroutineInit() {
  CoroutineActor* corotine = new CoroutineActor;
  //Coroutine *corotine = (Coroutine*)malloc(sizeof(*corotine));
  corotine->nco = 0;
  corotine->cap = DEFAULT_COROUTINE;
  corotine->running = -1;
  corotine->coctx = (CoroContext**)malloc(sizeof(CoroContext*) * corotine->cap);
  memset(corotine->coctx, 0, sizeof(CoroContext*) * corotine->cap);
  return corotine;
}

void CoroutineSchedule::CoroutineClose(CoroutineActor *corotine) {
  for (int i = 0; i < corotine->cap; i++) {
    CoroContext * coctx = corotine->coctx[i];
    if (coctx) {
      CoDelete(coctx);
    }
  }
  free(corotine->coctx);
  corotine->coctx = NULL;
  delete(corotine);
}

int CoroutineSchedule::CoroutineNew(CoroutineActor *corotine, CoroutineFunc func, void *ud) {
  CoroContext *coctx = CoNew(corotine, func, ud);
  if (corotine->nco >= corotine->cap) {
    int id = corotine->cap;
    corotine->coctx = (CoroContext**)realloc(corotine->coctx, corotine->cap * 2 * sizeof(CoroContext*));
    memset(corotine->coctx + corotine->cap, 0, sizeof(CoroContext *) * corotine->cap);
    corotine->coctx[corotine->cap] = coctx;
    corotine->cap *= 2;
    ++corotine->nco;
    return id;
  } else {
    int i;
    for (i = 0; i < corotine->cap; i++) {
      int id = (i + corotine->nco) % corotine->cap;
      if (corotine->coctx[id] == NULL) {
        corotine->coctx[id] = coctx;
        ++corotine->nco;
        return id;
      }
    }
  }
  return -1;
}

static void MainFunc(uint32_t low32, uint32_t hi32) {
  uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
  CoroutineActor *corotine = (CoroutineActor *)ptr;
  int id = corotine->running;
  CoroContext *coctx = corotine->coctx[id];
  coctx->func(corotine, coctx->ud);
  CoDelete(coctx);
  corotine->coctx[id] = NULL;
  --corotine->nco;
  corotine->running = -1;
}

void CoroutineSchedule::CoroutineResume(CoroutineActor* corotine, int id) {
  if (corotine->running != -1){
    WARN(logger_, "error running:" << corotine->running);
    return;
  }
  if (id < 0 || id >= corotine->cap){
    WARN(logger_, "error id" << id<<",cap:"<< corotine->cap);
    return;
  }

  CoroContext *coctx = corotine->coctx[id];
  if (coctx == NULL)
    return;
  int status = coctx->status;
  TRACE(logger_, "status:" << status << ",running id:" << corotine->running);
  switch (status) {
  case kCoroutineReady: {
    getcontext(&coctx->ctx);
    coctx->ctx.uc_stack.ss_sp = corotine->stack;
    coctx->ctx.uc_stack.ss_size = STACK_SIZE;
    coctx->ctx.uc_link = &corotine->main;
    corotine->running = id;
    coctx->status = kCoroutineRunning;
    uintptr_t ptr = (uintptr_t)corotine;
    makecontext(&coctx->ctx, (void(*)(void))MainFunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
    swapcontext(&corotine->main, &coctx->ctx);
    break; 
  }
  case kCoroutineSuspend:
    memcpy(corotine->stack + STACK_SIZE - coctx->size, coctx->stack, coctx->size);
    corotine->running = id;
    coctx->status = kCoroutineRunning;
    swapcontext(&corotine->main, &coctx->ctx);
    break; 
  default:
    WARN(logger_, "unkown status:" << status);
    break;
  }
}

static void SaveStack(CoroContext* coctx, char *top){
  char dummy = 0;
  if (top - &dummy >= STACK_SIZE){
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

void CoroutineSchedule::CoroutineYield(CoroutineActor * corotine) {
  int id = corotine->running;
  if (id <0){
    WARN(logger_, "error id:" << id);
    return;
  }
  CoroContext * coctx = corotine->coctx[id];
  if (corotine->stack >= (char *)&coctx){
    WARN(logger_, "error stack:" << corotine->stack);
    return;
  }
  SaveStack(coctx, corotine->stack + STACK_SIZE);
  coctx->status = kCoroutineSuspend;
  corotine->running = -1;
  swapcontext(&coctx->ctx, &corotine->main);
}

int CoroutineSchedule::CoroutineStatus(CoroutineActor* corotine, int id) {
  if (id<0 ||id >= corotine->cap){
    WARN(logger_,"error id:"<<id);
    return kCoroutineInvalid;
  }

  if (corotine->coctx[id] == NULL) {
    return kCoroutineInvalid;
  }
  return corotine->coctx[id]->status;
}

int CoroutineSchedule::CoroutineRunning(CoroutineActor * corotine) {
  return corotine->running;
}

}

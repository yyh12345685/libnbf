#pragma once

#include <ucontext.h>

#define STACK_SIZE (512*1024)
#define DEFAULT_COROUTINE 512

namespace bdf {

struct CoroutineActor;

struct CoroSchedule {
  char stack[STACK_SIZE];
  ucontext_t main_ctx;
  int nco;
  int cap;
  int running;
  CoroutineActor **coctxs;
};

typedef void(*CoroutineFunc)(void*);

struct CoroContext {
  CoroutineFunc func;
  void *ud;
  ucontext_t ctx;
  CoroSchedule* corotine;
  int ctx_cap;
  int size;
  int status;
  char *stack;
};

}

#pragma once

#include <ucontext.h>

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 32

namespace bdf {

struct CoroContext;
class CoroutineActor;

struct Coroutine {
  char stack[STACK_SIZE];
  ucontext_t main;
  int nco;
  int cap;
  int running;
  CoroContext **coctx;
};

typedef void(*CoroutineFunc)(CoroutineActor*, void*);

struct CoroContext {
  CoroutineFunc func;
  void *ud;
  ucontext_t ctx;
  Coroutine* corotine;
  int cap;
  int size;
  int status;
  char *stack;
};

}

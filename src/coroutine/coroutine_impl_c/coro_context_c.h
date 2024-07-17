#pragma once

#include <set>
#include <ucontext.h>
#include "coroutine/coro_context.h"
#include "coroutine/coroutine.h"

namespace bdf {

struct CoroContextList {
  char stack[STACK_SIZE];
  ucontext_t main_ctx;
  int nco;
  int cap;
  CoroContext* running = nullptr;
  // 实际存放的是派生类CoroContextc
  std::set <CoroContext*> coctxs; // 用set方便查找，又不会像hash set占用那么多内存
};

struct CoroContextc : public CoroContext {
  CoroutineFunc func;
  void *ud;
  ucontext_t ctx;
  // CoroContextList* ls;
  int ctx_cap;
  int size;
  int status;
  char *stack;
};

}

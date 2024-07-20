#pragma once

#include <set>
#include <ucontext.h>
#include "coroutine/coro_context.h"
#include "coroutine/coroutine.h"

namespace bdf {

struct CoroContextList {
  char stack[STACK_SIZE]; // 共享栈
  ucontext_t main_ctx;
  int nco;
  int cap;
  CoroContext* running = nullptr;
  // 实际存放的是派生类CoroContextc
  std::set <CoroContext*> coctxs; // 用set方便查找，又不会像hash set占用那么多内存
};

struct CoroContextc : public CoroContext {
  CoroutineFunc init_func = nullptr; // 支持初始化的时候传协程函数
  CoroutineFunc start_func = nullptr; //也支持启动协程的时候传协程函数
  void *ud;
  ucontext_t ctx;
  int ctx_cap;
  int size; // 栈大小
  int status;
  char *stack;
};

}

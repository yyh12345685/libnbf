#pragma once

#include <set>
#include <ucontext.h>
#include <stddef.h>
#include "coroutine/coro_context.h"
#include "coroutine/coroutine.h"

namespace bdf {

// 改为独享栈协程
// 这里共享栈有点问题：多线程时，协程切回去的时候恢复栈空间这块有空还可以看下
struct CoroContextList {
  //char stack[STACK_SIZE]; // 共享栈
  ucontext_t main_ctx;
  //int nco;
  int cap;
  int stack_size;
  CoroContext* running = nullptr;
  // 实际存放的是派生类CoroContextc
  std::set <CoroContext*> coctxs; // 用set方便查找，又不会像hash set占用那么多内存
};

struct CoroContextc : public CoroContext {
  CoroutineFunc init_func = nullptr; // 支持初始化的时候传协程函数
  CoroutineFunc start_func = nullptr; //也支持启动协程的时候传协程函数
  void *ud;
  ucontext_t* ctx;
  //ptrdiff_t ctx_cap;
  ptrdiff_t size; // 栈大小
  int status;
  char *stack;
};

}

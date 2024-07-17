#pragma once

namespace bdf {

class CoroutineActor;

struct CoroContext {
  virtual ~CoroContext() {}
  // 存放一个指针，做客户端时需要用
  CoroutineActor* actor = nullptr;
};

}

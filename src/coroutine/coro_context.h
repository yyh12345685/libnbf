#pragma once

namespace bdf {

class CoroutineActor;
class CoroRelease;

struct CoroContext {
  virtual ~CoroContext() {}
  // 存放一个指针，作为协程客户端时需要用
  CoroutineActor* actor = nullptr;

  CoroRelease* release = nullptr;
};

}

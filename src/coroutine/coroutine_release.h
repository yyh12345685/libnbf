#pragma once

namespace bdf {

class CoroContext;

// 给到外部一些使用协程的对象来释放资源
class CoroRelease {
    public:
    virtual bool ReleaseResource(CoroContext* coro) { return false; }
};

}

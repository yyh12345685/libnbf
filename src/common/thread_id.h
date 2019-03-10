
#pragma once

#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace bdf {

class ThreadId {
 public:
  static uint32_t Get() {
#ifdef SYS_gettid
    static thread_local uint32_t tid = syscall(SYS_gettid);
    return tid;
#elif __NR_gettid
    static thread_local uint32_t tid = syscall(__NR_gettid);
    return tid;
#else
  #error "SYS_gettid and __NR_gettid unavailable on this system"
#endif  
  }
};

}

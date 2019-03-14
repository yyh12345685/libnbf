
#pragma once

#include <atomic>

namespace bdf {

class SpinLock {
 public:
   SpinLock() {
    flag.clear(std::memory_order::memory_order_release);
  }

  void Lock() {
    while(flag.test_and_set(std::memory_order_acquire)){};
  }
  void UnLock() {
    flag.clear(std::memory_order_release);
  }
 private:
  std::atomic_flag flag;
};

class SpinLockScope {
 public:
   SpinLockScope(SpinLock& lock) : lock_(lock) {
    lock_.Lock();
  }
  ~SpinLockScope() {
    lock_.UnLock();
  }

 private:
   SpinLock& lock_;
};

}


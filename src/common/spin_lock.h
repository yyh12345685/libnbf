
#pragma once

#include <atomic>

namespace bdf {

namespace common {

class SpinLock {
 public:
   SpinLock() {
    flag.clear(std::memory_order::memory_order_release);
  }

  void lock() {
    while(flag.test_and_set(std::memory_order_acquire)){};
  }
  void unlock() {
    flag.clear(std::memory_order_release);
  }
 private:
  std::atomic_flag flag;
};

class SpinLockScope {
 public:
   SpinLockScope(SpinLock& lock) : lock_(lock) {
    lock_.lock();
  }
  ~SpinLockScope() {
    lock_.unlock();
  }

 private:
   SpinLock& lock_;
};

}

}


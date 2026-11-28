#pragma once

#include <stdint.h>
#include <time.h>
#include <atomic>

namespace bdf {
namespace common {

// Lets at most one event through per interval, across all threads, and
// counts the ones it held back. Meant for log lines on hot failure paths
// (overload drops, timeouts): logging every event there slows the I/O
// threads exactly when they are already behind.
//
//   static common::RateLimiter limiter;
//   uint64_t suppressed = 0;
//   if (limiter.Allow(&suppressed)) WARN(... << ",suppressed:" << suppressed);
class RateLimiter {
public:
  explicit RateLimiter(int64_t interval_seconds = 1)
    : interval_seconds_(interval_seconds) {
  }

  // True when the caller may log now. *suppressed is set to the number of
  // events held back since the previous allowed one.
  bool Allow(uint64_t* suppressed) {
    const int64_t now = time(nullptr);
    int64_t last = last_.load(std::memory_order_relaxed);
    if (now - last >= interval_seconds_ && last_.compare_exchange_strong(last, now)) {
      *suppressed = skipped_.exchange(0);
      return true;
    }
    skipped_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

private:
  const int64_t interval_seconds_;
  std::atomic<int64_t> last_{0};
  std::atomic<uint64_t> skipped_{0};
};

}
}

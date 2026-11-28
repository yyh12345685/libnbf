#pragma once

#include <stdint.h>
#include <atomic>
#include <functional>
#include <memory>

namespace bdf{

class EventMessage;

typedef std::function<void(EventMessage*)> InvokerCallback;

struct ContextBase{
public:
  ContextBase():
    monitor_id(0),
    callback(nullptr){
  }
  virtual ~ContextBase() {
    // The context lives from Invoke until the response (or failure) message
    // is destroyed, on every path: reply, timeout, drop, shutdown. Releasing
    // the in-flight slot here therefore cannot be missed.
    if (inflight) {
      inflight->fetch_sub(1, std::memory_order_relaxed);
    }
  }
  virtual void Destroy() {
    delete this;
  }
  uint64_t monitor_id;
  InvokerCallback callback;
  // In-flight counter of the Client that issued the Invoke (shared so it
  // stays valid if the Client is deleted before its last response).
  std::shared_ptr<std::atomic<int64_t> > inflight;
};

}

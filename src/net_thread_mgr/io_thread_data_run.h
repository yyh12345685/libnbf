
#pragma once

#include <atomic>
#include <stddef.h>
#include "event/thread_data_run.h"
#include "common/logger.h"

namespace bdf{
   
struct HandleData;
class EventMessage;

class IoThreadDataRun:public ThreadDataRun{
public:
  IoThreadDataRun(HandleData* hd_data){
      hd_data_ = hd_data;
  }
  virtual ~IoThreadDataRun();

  virtual void CallRun();
  void PutMessage(EventMessage* message);

  // Messages waiting in this I/O thread's input queue (not counting the
  // batch being processed). Read without the lock; used for backpressure.
  size_t PendingSize() const { return pending_.load(std::memory_order_relaxed); }

protected:
  void Handle(EventMessage* message);

  void HandleIoMessageEvent(EventMessage* message);
  void HandleIoMessageFailed(EventMessage* message);
  void HandleIoActiveCloseEvent(EventMessage* message);

  HandleData* hd_data_;
private:
  std::atomic<size_t> pending_{0};
  LOGGER_CLASS_DECL(logger_);
};

} // namespace bdf

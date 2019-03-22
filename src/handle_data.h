#pragma once

#include "handle.h"

#include <queue>
#include <mutex>

namespace bdf{

class EventMessage;
class Task;

struct HandleData {
  ~HandleData() {
    if (handle_ != nullptr){
      delete handle_;
    }
  }
  std::queue<EventMessage*> data_;

  std::queue<Task*> task_;

  std::mutex lock_;
  bool is_run = false;

  Handler* handle_ = nullptr;
};

}

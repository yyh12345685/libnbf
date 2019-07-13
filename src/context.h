#pragma once

#include <functional>

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
  }
  virtual void Destroy() { 
    delete this; 
  }
  uint64_t monitor_id;
  InvokerCallback callback;
};

}

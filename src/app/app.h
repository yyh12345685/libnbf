#pragma once

#include <functional>
#include "app/appbase.h"

namespace bdf{

template<typename ServiceHandlerType>
class Application : public AppBase {
public:
  typedef std::function<int()> AppFunc;

  Application():
    on_start_func_(nullptr),
    on_after_start_func_(nullptr),
    on_stop_func_(nullptr) {

  }

  Application& SetOnStart(const AppFunc& func) {
    on_start_func_ = func;
    return *this;
  }

  Application& SetOnAfterStart(const AppFunc& func) {
    on_after_start_func_ = func;
    return *this;
  }

  Application& SetOnStop(const AppFunc& func) {
    on_stop_func_ = func;
    return *this;
  }

protected:
  virtual ServiceHandlerType* CreateServiceHandler() {
    return new ServiceHandlerType();
  }

  virtual int OnStart() {
    if (on_start_func_) {
      return on_start_func_();
    }
    return 0;
  }
  virtual int OnAfterStart() {
    if (on_after_start_func_) {
      return on_after_start_func_();
    }
    return 0;
  }

  virtual int OnStop() {
    if (on_stop_func_) {
      return on_stop_func_();
    }
    return 0;
  }

private:
  AppFunc on_start_func_;
  AppFunc on_after_start_func_;
  AppFunc on_stop_func_;
};


}

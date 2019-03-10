#pragma once

namespace bdf{

template<typename ServiceHandlerType>
class Application : public AppBase {
public:
  typedef std::function<int()> DelegateFunc;

  GenericApplication& SetOnStart(const DelegateFunc& func) {
    on_start_func_ = func;
    return *this;
  }

  GenericApplication& SetOnStop(const DelegateFunc& func) {
    on_stop_func_ = func;
    return *this;
  }

  GenericApplication& SetOnFinish(const DelegateFunc& func) {
    on_finish_func_ = func;
    return *this;
  }

protected:
  virtual ServiceHandler* CreateServiceHandler() {
    return new ServiceHandlerType(GetIOService());
  }

  virtual int OnStart() {
    if (on_start_func_) {
      return on_start_func_();
    }
    return 0;
  }

  virtual int OnStop() {
    if (on_stop_func_) {
      return on_stop_func_();
    }
    return 0;
  }

  virtual int OnFinish() {
    if (on_finish_func_) {
      return on_finish_func_();
    }
    return 0;
  }

private:
  DelegateFunc on_start_func_;
  DelegateFunc on_stop_func_;
  DelegateFunc on_finish_func_;
};

}

#pragma once

namespace bdf{

class HandleData;

class Handler {
public:
  virtual ~Handler(){
  }

  void SetHandlerId(int32_t handler_id) { handler_id_ = handler_id; }
  int32_t GetHandlerId() const { return handler_id_; }

  virtual void Run(HandleData* data) =0;
private:
  int32_t handler_id_ = -1;
};

}

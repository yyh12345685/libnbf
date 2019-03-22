#pragma once

namespace bdf{

class HandleData;

class Handler {
public:
  Handler(){}
  virtual ~Handler(){
  }

  virtual void Run(HandleData* data) =0;
};

}


#pragma once

namespace bdf{
  
class ThreadDataRun{
public:
  virtual ~ThreadDataRun(){}
  virtual void CallRun() = 0;
};


}

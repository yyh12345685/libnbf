
#pragma once

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

protected:
  void Handle(EventMessage* message);

  void HandleIoMessageEvent(EventMessage* message);
  void HandleIoMessageFailed(EventMessage* message);
  void HandleIoActiveCloseEvent(EventMessage* message);

  HandleData* hd_data_; 
private:
  LOGGER_CLASS_DECL(logger_);
};

} // namespace bdf

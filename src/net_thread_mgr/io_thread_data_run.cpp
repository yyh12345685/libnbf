#include <time.h>
#include <queue>
#include "io_thread_data_run.h"
#include "handle_data.h"
#include "message_base.h"
#include "service/service_manager.h"
#include "net/connect.h"
#include "net/server_connect.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/acceptor_thread.h"
#include "common/thread_id.h"
#include "common/time.h"
#include "monitor/matrix_scope.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, IoThreadDataRun);

IoThreadDataRun::~IoThreadDataRun(){
  if(nullptr  != hd_data_){
    delete hd_data_;
  }
}

void IoThreadDataRun::PutMessage(EventMessage* message) {
  hd_data_->data_lock_.lock();
  hd_data_->data_.emplace(message);
  hd_data_->data_lock_.unlock();
  //hd_data_->PushData(message);
}

void IoThreadDataRun::CallRun() {
  if (hd_data_->data_.empty()){
    return;
  }
  
  static time_t log_time = time(NULL);
  std::queue<EventMessage*> temp;
  hd_data_->data_lock_.lock();
  temp.swap(hd_data_->data_);
  hd_data_->data_lock_.unlock();
  /*if(!hd_data_->PopAllData(temp, 1)) {
    return;
  }*/

  //如有必要这里也可以增加队列超过一定长度的过载保护
  size_t handle_size = temp.size();
  while (!temp.empty()) {
    //for debug
    time_t cur_time = time(NULL);
    if ((cur_time - log_time) > 60 && handle_size == temp.size()) {
      INFO(logger_, "ThreadId:" << ThreadId::Get() << ",msg size:"
        << hd_data_->data_.size() << ",handle size:" << temp.size());
      log_time = cur_time;
    }

    EventMessage *msg = temp.front();
    temp.pop();
    Handle(msg);
  }
}

void IoThreadDataRun::Handle(EventMessage* message){
  TRACE(logger_, "IoThreadDataRun::Handle msg:" << *message);
  switch (message->type_io_event) {
  case MessageType::kIoHandleEventMsg:
    HandleIoMessageEvent(message);
    break;
  case MessageType::kIoActiveCloseEvent:
    HandleIoActiveCloseEvent(message);
    break;
  default:
    WARN(logger_, "IoThreadDataRun unknown message:" << message->type_id);
    MessageFactory::Destroy(message);
    break;
  }
}

void IoThreadDataRun::HandleIoMessageEvent(EventMessage* message){
  TRACE(logger_, "HandleIoMessageEvent");
  //客户端过载保护
  if (message->direction == EventMessage::kOnlySend
    || message->direction == EventMessage::kOutgoingRequest){
    uint64_t birth_to_now = Time::GetCurrentClockTime() - message->birthtime;
    if (birth_to_now > INNER_QUERY_SEND_PROTECT_TIME) {
      monitor::MatrixCounter::Add("client_send_queue_overload", 1);
      //从发送到io handle处理超过100ms,过载保护
      INFO(logger_, "birth_to_now:" << birth_to_now << ",msg:" << *message);
      HandleIoMessageFailed(message);
      return;
    }
  }
  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  
  if (!con /*|| reg_con != con*/) {
    ERROR(logger_, "error,con:"<< con /*<<",reg_con:"<< reg_con*/);
    HandleIoMessageFailed(message);
    return;
  }

  if (message->direction == EventMessage::kOutgoingResponse) {
    // A server reply: its connection may have closed (and be deleted 90s
    // later) while the handler worked. Check before dereferencing; this runs
    // on the connection's own I/O thread, so it cannot close meanwhile.
    AcceptorThread* acceptor =
      service::GetServiceManager().GetNetThreadManager()->GetAcceptThread();
    if (nullptr == acceptor
      || !acceptor->IsLiveConnect(static_cast<ServerConnect*>(con))) {
      TRACE(logger_, "reply for a closed connection dropped");
      HandleIoMessageFailed(message);
      return;
    }
  }

  if (con->GetFd() < 0) {
    // Closed: encoding would only pile data into a buffer nobody sends.
    HandleIoMessageFailed(message);
    return;
  }

  if (0 != con->EncodeMsg(message)){
    HandleIoMessageFailed(message);
    return;
  }
  //单向消息encode之后即可释放，双向消息会在返回或者超时的时候释放
  //server答复的消息需要在这里释放掉
  if (message->direction == EventMessage::kOnlySend
    || message->direction == EventMessage::kOutgoingResponse) {
    MessageFactory::Destroy(message);
  }

  con->OnWrite();
}

void IoThreadDataRun::HandleIoMessageFailed(EventMessage* message) {
  if (message->direction == EventMessage::kOutgoingRequest) {
    message->status = EventMessage::kInternalFailure;
    service::GetServiceManager().SendToServiceHandle(message);
  } else {
    MessageFactory::Destroy(message);
  }
}

void IoThreadDataRun::HandleIoActiveCloseEvent(EventMessage* message){
  TRACE(logger_, "HandleIoActiveCloseEvent");

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    MessageFactory::Destroy(message);
    WARN(logger_, "HandleIoActiveCloseEvent descriptor is not Connecting");
    return;
  }

  MessageFactory::Destroy(message);
  con->Destroy();
}

}

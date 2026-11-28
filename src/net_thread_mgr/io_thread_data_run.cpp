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
#include "common/rate_limiter.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, IoThreadDataRun);

IoThreadDataRun::~IoThreadDataRun(){
  if (hd_data_) {
    while (!hd_data_->data_.empty()) {
      MessageFactory::Destroy(hd_data_->data_.front());
      hd_data_->data_.pop();
    }
    delete hd_data_;
  }
}

void IoThreadDataRun::PutMessage(EventMessage* message){
  hd_data_->data_lock_.lock();
  hd_data_->data_.emplace(message);
  pending_.store(hd_data_->data_.size(), std::memory_order_relaxed);
  hd_data_->data_lock_.unlock();
}

void IoThreadDataRun::CallRun(){
  static thread_local time_t log_time = time(NULL);
  std::queue<EventMessage*> temp;
  // Check emptiness under the lock too: producers push concurrently and an
  // unlocked std::queue read is a data race.
  hd_data_->data_lock_.lock();
  if (hd_data_->data_.empty()){
    hd_data_->data_lock_.unlock();
    return;
  }
  temp.swap(hd_data_->data_);
  pending_.store(0, std::memory_order_relaxed);
  hd_data_->data_lock_.unlock();

  //如有必要这里也可以增加队列超过一定长度的过载保护
  size_t handle_size = temp.size();
  while (!temp.empty()) {
    //for debug
    time_t cur_time = time(NULL);
    if ((cur_time - log_time) > 60 && handle_size == temp.size()) {
      INFO(logger_, "ThreadId:" << ThreadId::Get()
        << ",handle size:" << temp.size());
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
  Connecting* con = Connecting::DescriptorConnect(message->descriptor_id);
  if (!con /*|| reg_con != con*/) {
    ERROR(logger_, "error,con:"<< con /*<<",reg_con:"<< reg_con*/);
    HandleIoMessageFailed(message);
    return;
  }
  const bool client_send = message->direction == EventMessage::kOnlySend
    || message->direction == EventMessage::kOutgoingRequest
    || message->direction == EventMessage::kSendNoCareResponse;
  //客户端过载保护
  if (client_send){
    uint64_t birth_to_now = Time::GetCurrentClockTime() - message->birthtime;
    if (birth_to_now > INNER_QUERY_SEND_PROTECT_TIME) {
      monitor::MatrixCounter::Add("client_send_queue_overload", 1);
      //从发送到io handle处理超过200ms,过载保护
      // Rate limited: one line per drop (with the whole message) made the
      // I/O thread slower exactly while it was behind, so the backlog grew.
      static common::RateLimiter overload_log;
      uint64_t suppressed = 0;
      if (overload_log.Allow(&suppressed)) {
        WARN(logger_, "client send queue overload, drop, birth_to_now:" << birth_to_now
          << ",suppressed since last log:" << suppressed << ",msg:" << *message);
      }
      con->OnSendDropped();
      HandleIoMessageFailed(message);
      return;
    }
  }

  if (message->direction == EventMessage::kOutgoingResponse) {
    // A server reply: its connection may have closed (and be deleted 90s
    // later) while the handler worked. Check before dereferencing; this runs
    // on the connection's own I/O thread, so it cannot close meanwhile.
    AcceptorThread* acceptor =
      service::GetServiceManager().GetNetThreadManager()->GetAcceptThread();
    // The generation check catches a new connection that reuses the address
    // of the one this reply belongs to (it is deleted 90s after closing).
    if (nullptr == acceptor
      || !acceptor->IsLiveConnect(static_cast<ServerConnect*>(con))
      || (uint16_t)(con->GetConnectId() & 0xFFFF)
         != Connecting::DescriptorGen(message->descriptor_id)) {
      TRACE(logger_, "reply for a closed connection dropped");
      HandleIoMessageFailed(message);
      return;
    }
    static_cast<ServerConnect*>(con)->SendReply(message);
    return;
  }

  if (con->GetFd() < 0) {
    // Closed: encoding would only pile data into a buffer nobody sends.
    if (client_send) con->OnSendDropped();
    HandleIoMessageFailed(message);
    return;
  }

  if (0 != con->EncodeMsg(message)){
    HandleIoMessageFailed(message);
    return;
  }
  if (message->direction == EventMessage::kOnlySend
      && !con->GetProtocol()->IsSynchronous()) {
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

  Connecting* con = Connecting::DescriptorConnect(message->descriptor_id);
  if (!con) {
    MessageFactory::Destroy(message);
    WARN(logger_, "HandleIoActiveCloseEvent descriptor is not Connecting");
    return;
  }

  MessageFactory::Destroy(message);
  con->Destroy();
}

}

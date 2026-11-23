
#include <atomic>
#include "net/server_connect.h"
#include "common/time.h"
#include "service/service_manager.h"
#include "net/connect_manager.h"
#include "net_thread_mgr/net_thread_mgr.h"
#include "net_thread_mgr/io_threads.h"
#include "protocol/http/http_protocol.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, ServerConnect);
static const uint64_t kMaxPendingHttpRequests = 1024;
// A new message's sequence_id: the reply was not built from the request.
static const uint64_t kNoSequence = 0xffffffffffffffffULL;
// Timer tags (function_data) to tell the two timers apart.
static int kLingerTag = 0;
static int kReplyWaitTag = 0;

ServerConnect::ServerConnect(){
  static std::atomic<int64_t> server_connect_id(0);
  SetConnectId(server_connect_id.fetch_add(1, std::memory_order_relaxed));
}

ServerConnect::~ServerConnect(){
  // Shutdown deletes still-open connections (delay release manager); a
  // pending linger timer would otherwise fire on freed memory.
  CancelLinger();
  CancelReplyWait();
  ClearPendingReplies();
}

bool ServerConnect::IsOrdered() {
  if (ordered_state_ < 0) {
    ordered_state_ = dynamic_cast<HttpProtocol*>(GetProtocol()) != nullptr ? 1 : 0;
  }
  return ordered_state_ == 1;
}

void ServerConnect::OnClose() {
  if (fd_ < 0) {
    return;
  }
  //as server,if connect is closed,can delete Connecting
  //as client,if connect is closed,only closed fd
  CancelLinger();
  CancelReplyWait();
  RegisterDel(fd_);
  Clean();
  //if (ConnectManager::Instance().GetConnect((uint64_t)this)){
  //  ConnectManager::Instance().UnRegisterConnect((uint64_t)this);
    Destroy();
  //}
}

void ServerConnect::Destroy(){
  // Also reached from kIoActiveCloseEvent with the fd still open: unregister
  // and close it first, or the fd stays registered with a pointer that
  // dangles once the delayed delete runs.
  CancelLinger();
  CancelReplyWait();
  if (fd_ >= 0) {
    RegisterDel(fd_);
    Clean();
  }
  if (released_) {
    return; // OnClose and an active close both end here
  }
  released_ = true;
  ClearPendingReplies();
  service::GetServiceManager().ReleaseServerCon(this);
  //BDF_DELETE(this);
}

void ServerConnect::OnDecodeMessage(EventMessage* message) {
  if (close_after_reply_) {
    MessageFactory::Destroy(message);
    return;
  }
  const bool ordered = IsOrdered();
  if (ordered) {
    if (next_request_id_ - next_reply_id_ >= kMaxPendingHttpRequests) {
      WARN(logger_, "too many pipelined http requests without reply, close,fd:"
        << fd_ << ",ip:" << ip_ << ",port:" << port_);
      MessageFactory::Destroy(message);
      OnClose();
      return;
    }
    message->sequence_id = next_request_id_++;
    ++in_flight_;
  }
  if (message->type_id == MessageType::kHeartBeatMessage) {
    EventMessage* heartbeat_response = GetProtocol()->HeartBeatResponse(message);
    if (ordered) {
      SendReply(heartbeat_response);
      return;
    }
    // Encode copies into outbuf_, so the message is freed on success too
    // (it used to leak one message per heartbeat / favicon request).
    int ret = EncodeMsg(heartbeat_response);
    MessageFactory::Destroy(heartbeat_response);
    if (0 == ret) {
      OnWrite(); // decode runs on this connection's I/O thread
    }
    return;
  }

  message->birthtime = Time::GetCurrentClockTime();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingRequest;
  message->descriptor_id = MakeDescriptor(this, GetConnectId());

  if (!ordered) ++in_flight_;
  service::GetServiceManager().SendToServiceHandle(message);
}

void ServerConnect::SendReply(EventMessage* message) {
  if (close_after_reply_) {
    MessageFactory::Destroy(message);
    return;
  }
  if (!IsOrdered()) {
    const bool dropped = message->event_mask & MessageBase::kEventDropNotice;
    const bool encoded = !dropped && EncodeMsg(message) == 0;
    MessageFactory::Destroy(message);
    if (in_flight_ > 0) --in_flight_;
    if (encoded) OnWrite();
    else MaybeFinishHalfClose();
    return;
  }

  uint64_t sequence = message->sequence_id;
  if (sequence == kNoSequence && next_request_id_ - next_reply_id_ == 1
      && pending_replies_.empty()) {
    // Reply not built with InitReply(request): with exactly one request
    // outstanding it can only belong to that one (the old behaviour).
    sequence = next_reply_id_;
  }
  if (sequence >= next_request_id_) {
    // Order unknown: sending it anywhere could answer the wrong request.
    WARN(logger_, "http reply without a valid request sequence (build it with "
      "InitReply(request, ...)), close,fd:" << fd_ << ",sequence:" << message->sequence_id
      << ",outstanding:" << (next_request_id_ - next_reply_id_));
    MessageFactory::Destroy(message);
    OnClose();
    return;
  }
  if (sequence < next_reply_id_ || !pending_replies_.emplace(sequence, message).second) {
    WARN(logger_, "duplicate http reply dropped,fd:" << fd_ << ",sequence:" << sequence);
    MessageFactory::Destroy(message);
    return;
  }

  const uint64_t head_before = next_reply_id_;
  while (!pending_replies_.empty() && pending_replies_.begin()->first == next_reply_id_) {
    auto entry = pending_replies_.begin();
    EventMessage* reply = entry->second;
    pending_replies_.erase(entry);
    const bool dropped = reply->event_mask & MessageBase::kEventDropNotice;
    if (dropped) {
      MessageFactory::Destroy(reply);
      ClearPendingReplies();
      close_after_reply_ = true;
      StartLingerTimer();
      break;
    }
    const bool encoded = EncodeMsg(reply) == 0;
    MessageFactory::Destroy(reply);
    if (!encoded) {
      WARN(logger_, "encode http reply failed, close,fd:" << fd_);
      OnClose();
      return;
    }
    ++next_reply_id_;
    --in_flight_;
  }
  // Progress made: the wait for the (new) head reply starts now.
  if (next_reply_id_ != head_before) CancelReplyWait();
  UpdateReplyWaitTimer();
  OnWrite();
}

void ServerConnect::UpdateReplyWaitTimer() {
  if (pending_replies_.empty() || close_after_reply_) {
    CancelReplyWait();
    return;
  }
  if (reply_wait_timer_ != 0) return; // already waiting for this head
  TimerData td;
  td.time_out_ms = kReplyWaitMs;
  td.time_proc = this;
  td.function_data = &kReplyWaitTag;
  NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
  reply_wait_timer_ = net->StartTimer(td, net->GetIoThreads()->GetIoThreadId(this));
}

void ServerConnect::CancelReplyWait() {
  if (reply_wait_timer_ != 0) {
    NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
    if (net) net->CancelTimer(reply_wait_timer_, net->GetIoThreads()->GetIoThreadId(this));
    reply_wait_timer_ = 0;
  }
}

void ServerConnect::ClearPendingReplies() {
  for (const auto& entry : pending_replies_) {
    MessageFactory::Destroy(entry.second);
  }
  pending_replies_.clear();
}

void ServerConnect::OnPeerClosed(){
  if (peer_closed_ || fd_ < 0) {
    return;
  }
  peer_closed_ = true;
  if (in_flight_ == 0 && outbuf_.GetReadingSize() == 0) {
    graceful_close_ = true;
    OnClose();
    return;
  }
  StartLingerTimer();
}

void ServerConnect::StartLingerTimer(){
  if (linger_timer_ != 0) return;
  TimerData td;
  td.time_out_ms = kHalfCloseLingerMs;
  td.time_proc = this;
  td.function_data = &kLingerTag;
  NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
  linger_timer_ = net->StartTimer(td, net->GetIoThreads()->GetIoThreadId(this));
}

void ServerConnect::OnTimer(void* function_data){
  if (function_data == &kReplyWaitTag) {
    reply_wait_timer_ = 0;
    if (fd_ >= 0) {
      WARN(logger_, "http reply " << next_reply_id_ << " missing for " << kReplyWaitMs
        << "ms while later replies wait, close,fd:" << fd_ << ",ip:" << ip_);
      OnClose();
    }
    return;
  }
  linger_timer_ = 0;
  if (fd_ >= 0) {
    INFO(logger_, "connection timed out waiting for replies to drain,fd:" << fd_);
    OnClose();
  }
}

void ServerConnect::CancelLinger(){
  if (linger_timer_ != 0) {
    NetThreadManager* net = service::GetServiceManager().GetNetThreadManager();
    if (net) net->CancelTimer(linger_timer_, net->GetIoThreads()->GetIoThreadId(this));
    linger_timer_ = 0;
  }
}

void ServerConnect::OnWrite(){
  Connecting::OnWrite();
  if (close_after_reply_ && fd_ >= 0 && outbuf_.GetReadingSize() == 0) {
    graceful_close_ = true;
    OnClose();
    return;
  }
  MaybeFinishHalfClose();
}

void ServerConnect::MaybeFinishHalfClose(){
  if (peer_closed_ && fd_ >= 0 && in_flight_ == 0 && outbuf_.GetReadingSize() == 0) {
    graceful_close_ = true; // let the kernel deliver what is queued
    OnClose();
  }
}

int ServerConnect::RegisterDel(int fd){
  if (fd < 0){
    return -1;
  }
  return service::GetServiceManager().EventDel(this, fd);
}

}

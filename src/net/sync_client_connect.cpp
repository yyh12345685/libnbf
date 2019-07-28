
#include "net/sync_client_connect.h"
#include "service/io_service.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, SyncClientConnect);

SyncClientConnect::SyncClientConnect(
  const uint32_t& timeout_ms,
  const uint32_t& heartbeat_ms,
  const bool& sigle_send_sigle_recv) :
  ClientConnect(timeout_ms,heartbeat_ms),
  sync_sequence_(this,timeout_ms),
  sigle_send_sigle_recv_(sigle_send_sigle_recv){
}

SyncClientConnect::~SyncClientConnect(){
}

void SyncClientConnect::OnDecodeMessage(EventMessage* message) {
  EventMessage* keeper_message = sync_sequence_.Get();
  if (!keeper_message) {
    WARN(logger, "OnDecodeMessage keeper not found, or oneway message.");
    MessageFactory::Destroy(message);
    return;
  }

  if (keeper_message->type_id == MessageType::kHeartBeatMessage
    || keeper_message->direction == EventMessage::kSendNoCareResponse
    || keeper_message->direction == EventMessage::kOnlySend) {
    TRACE(logger, "OnDecodeMessage heartbeat or no care response:" << *keeper_message);
    MessageFactory::Destroy(message);
    MessageFactory::Destroy(keeper_message);
    return;
  }

  if (sigle_send_sigle_recv_  && 1 != sync_sequence_.Size()){
    INFO(logger, "11111111,size:"<< sync_sequence_.Size());
  }

  message->birthtime = Time::GetMillisecond();
  message->status = EventMessage::kStatusOK;
  message->direction = EventMessage::kIncomingResponse;
  message->sequence_id = keeper_message->sequence_id;
  message->coroutine_id = keeper_message->coroutine_id;
  message->handle_id = keeper_message->handle_id;
  message->descriptor_id = (int64_t)((void*)(this));
  if (nullptr != keeper_message->ctx && nullptr != keeper_message->ctx->callback) {
    message->ctx = BDF_NEW(ContextBase);
    message->ctx->callback = keeper_message->ctx->callback;
    message->ctx->monitor_id = keeper_message->ctx->monitor_id;
  }
  TRACE(logger, "SyncClientChannel::OnDecodeMessage " << *message);

  MessageFactory::Destroy(keeper_message);
  IoService::GetInstance().SendToServiceHandle(message);
}

int SyncClientConnect::EncodeMsg(EventMessage* message){

  if (GetStatus() != kConnected) {
    DEBUG(logger, "SyncClientConnect::EncodeMsg ChannelBroken:" << GetStatus());
    SetBuzy(false);
    return -1;
  }
  TRACE(logger, "EncodeMsg msg type:" << (int)(message->type_id)
    << ",direction:" << (int)(message->direction));

  if (sigle_send_sigle_recv_ && sync_sequence_.Size()>1) {
    //可能还有一个心跳包
    INFO(logger, "22222222,size:"<<sync_sequence_.Size());
  }

  //test_lock1_.lock();
  if (!GetProtocol()->Encode(message, &outbuf_)) {
    WARN(logger, "SyncClientConnect::Encode() encode fail");
    //test_lock1_.unlock();
    SetBuzy(false);
    return -2;
  }
  //test_lock1_.unlock();

  //同步请求不存真正only send的情况，一定回有应答，比如http协议和redis协议
  //有应答的情况需要将消息保存起来，这里属于有应答的情况
  //if (message->direction != EventMessage::kSendNoCareResponse
  //  && message->direction != EventMessage::kOnlySend){
    if (0 != sync_sequence_.Put(message)) {
      WARN(logger, "SyncClientConnect::EncodeMsg put fail");
      SetBuzy(false);
      return -3;
    }
  //}
 
  return 0;
}

void SyncClientConnect::CleanSyncClient(){
  if (GetStatus() == kConnected) {
    CleanClient();
  }
  SetBuzy(false);
}

void SyncClientConnect::CleanSequenceQueue(){
  std::list<EventMessage*> clear_list = sync_sequence_.Clear();
  EventMessage* message = clear_list.front();
  while (!clear_list.empty() && message) {
    clear_list.pop_front();
    DoSendBack(message, EventMessage::kInternalFailure);
    message = clear_list.front();
  }

  sync_sequence_.ClearTimer();
}

int SyncClientConnect::DecodeMsg(){
  bool failed = false;
  while (sync_sequence_.Size()) {
    //test_lock_.lock();
    EventMessage* msg = protocol_->Decode(inbuf_, failed);
    //test_lock_.unlock();
    if (failed) {
      failed = true;
      TRACE(logger_, "Connecting::Decode,base->Decode failed.");
      if (msg != nullptr) {
        MessageFactory::Destroy(msg);
      }
      break;
    }

    if (nullptr == msg) {
      TRACE(logger_, "event is nullptr.");
      break;
    }

    OnDecodeMessage(msg);
  }
  SetBuzy(false);
  if (failed) {
    return -1;
  }

  return 0;
}

}



#include "net/sync_client_connect.h"
#include "service/io_service.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, SyncClientConnect);

SyncClientConnect::SyncClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  ClientConnect(timeout_ms,heartbeat_ms),
  sync_sequence_(this,timeout_ms){
}

SyncClientConnect::~SyncClientConnect(){
}

void SyncClientConnect::OnDecodeMessage(EventMessage* message) {
  EventMessage* keeper_message = sync_sequence_.Get();
  if (!keeper_message) {
    WARN(logger, "OnDecodeMessage keeper not found, or oneway message.");
    MessageFactory::Destroy(message);
    CleanSyncClient();
    return;
  }

  if (keeper_message->type_id == MessageType::kHeartBeatMessage
    || keeper_message->direction == EventMessage::kOneway) {
    TRACE(logger, "SyncClientChannel::OnDecodeMessage heartbeat or oneway:" << *message);
    MessageFactory::Destroy(message);
    MessageFactory::Destroy(keeper_message);
    sync_sequence_.Fire();
    return;
  }

  message->birthtime = Time::GetMillisecond();
  message->status = EventMessage::kStatusOK;
  message->direction = EventMessage::kIncomingResponse;
  message->sequence_id = keeper_message->sequence_id;
  message->descriptor_id = (int64_t)((void*)(this));

  TRACE(logger, "SyncClientChannel::OnDecodeMessage " << *message);

  MessageFactory::Destroy(keeper_message);
  IoService::GetInstance().SendToServiceHandle(message);

  sync_sequence_.Fire();
}

int SyncClientConnect::EncodeMsg(EventMessage* message){
  if (GetStatus() != kConnected) {
    DEBUG(logger, "SyncClientConnect::EncodeMsg ChannelBroken:" << GetStatus());
    return -1;
  }

  if (message->direction != EventMessage::kOneway){
    if (0 != sync_sequence_.Put(message)) {
      WARN(logger, "SyncClientConnect::EncodeMsg put fail");
      return -2;
    }
  }

  FireMessage();
  return 0;
}

void SyncClientConnect::FireMessage(){
  while (true) {
    EventMessage* fire_message = sync_sequence_.Fire();
    if (!fire_message) {
      return;
    }

    if (!GetProtocol()->Encode(fire_message,&outbuf_)) {
      WARN(logger, "SyncClientConnect::FireMessage() encode fail");
      DoSendBack(fire_message, EventMessage::kStatusEncodeFail);
      sync_sequence_.Get();
      continue;
    }

    //OnWrite();
    break;
  }
}

void SyncClientConnect::CleanSyncClient(){
  if (GetStatus() == kConnected) {
    CleanClient();
  }
}

void SyncClientConnect::OnTimeout(EventMessage* msg){
  if (nullptr == msg){
    return;
  }
  DEBUG(logger, "SyncClientConnect::OnTimeout, message:" << *msg);
  DoSendBack(msg, EventMessage::kStatusTimeout);
}

void SyncClientConnect::CleanSequenceQueue(){
  std::list<EventMessage*> clear_list = sync_sequence_.Clear();
  EventMessage* message = clear_list.front();
  while (!clear_list.empty() && message) {
    clear_list.pop_front();
    DoSendBack(message, EventMessage::kInternalFailure);
    message = clear_list.front();
  }
}

int SyncClientConnect::DecodeMsg(){
  bool failed = false;
  while (sync_sequence_.Fired()) {
    EventMessage* msg = protocol_->Decode(inbuf_, failed);
    if (failed) {
      TRACE(logger_, "Connecting::Decode,base->Decode failed.");
      if (msg != NULL) {
        MessageFactory::Destroy(msg);
      }
      break;
    }

    if (NULL == msg) {
      TRACE(logger_, "event is NULL.");
      break;
    }

    OnDecodeMessage(msg);
  }
  if (failed) {
    return -1;
  }

  return 0;
}

}


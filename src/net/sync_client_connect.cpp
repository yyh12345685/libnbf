
#include "net/sync_client_connect.h"
#include "service/io_service.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger, SyncClientConnect);

SyncClientConnect::SyncClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  ClientConnect(timeout_ms,heartbeat_ms),
  sync_sequence_(timeout_ms){
}

SyncClientConnect::~SyncClientConnect(){
}

void SyncClientConnect::OnDecodeMessage(EventMessage* message) {
  EventMessage* keeper_message = sync_sequence_.Get();
  if (!keeper_message) {
    WARN(logger, "SyncClientConnect::OnDecodeMessage keeper not found, might timeout");
    MessageFactory::Destroy(message);
    CleanClient();
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

  if (0 != sync_sequence_.Put(message)){
    WARN(logger, "SyncClientConnect::EncodeMsg put fail");
    return -2;
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

    OnWrite();
    break;
  }
}

void SyncClientConnect::OnTimeout() {
  std::list<EventMessage*> tmo = sync_sequence_.Timeout();
  if (tmo.empty()) {
    DEBUG(logger, "SyncClientConnect::OnTimeout,tmo is empty.");
    CleanClient();
  }

  EventMessage* message = tmo.front();
  while (!tmo.empty() && message) {
    tmo.pop_front();
    DEBUG(logger, "SyncClientConnect::OnTimeout, message:"<< *message);
    DoSendBack(message, EventMessage::kStatusTimeout);
    message = tmo.front();
  }

  StartTimeoutTimer();
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


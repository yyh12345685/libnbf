#include "net/async_client_connect.h"
#include "service/io_service.h"
#include "common/time.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, AsyncClientConnect);

AsyncClientConnect::AsyncClientConnect(
  uint32_t timeout_ms, uint32_t heartbeat_ms):
  ClientConnect(timeout_ms,heartbeat_ms),
  async_sequence_(this,timeout_ms){
}

AsyncClientConnect::~AsyncClientConnect(){
}

//由slave线程触发
void AsyncClientConnect::OnDecodeMessage(EventMessage* message){
  EventMessage* keeper_message = async_sequence_.Get(message->sequence_id);
  if (!keeper_message) {
    DEBUG(logger, "OnDecodeMessage sequence not found or oneway msg"
      << ", sequence_id:" << message->sequence_id);
    MessageFactory::Destroy(message);
    return;
  }

  if (keeper_message->type_id == MessageType::kHeartBeatMessage) {
    TRACE(logger, "AsyncClientConnect::OnDecodeMessage heartbeat:" << *message);
    MessageFactory::Destroy(message);
    MessageFactory::Destroy(keeper_message);
    return;
  }

  message->birthtime = Time::GetMillisecond();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingResponse;
  message->sequence_id = keeper_message->sequence_id;
  message->descriptor_id = (int64_t)((void*)(this));

  TRACE(logger, "AsyncClientConnect::OnDecodeMessage " << *message);
  MessageFactory::Destroy(keeper_message);

  IoService::GetInstance().SendToServiceHandle(message);
}

//由io handle线程触发
int AsyncClientConnect::EncodeMsg(EventMessage* message){
  if (GetStatus() != kConnected) {
    DEBUG(logger, "AsyncClientConnect::EncodeMsg ChannelBroken:" << GetStatus());
    return -1;
  }

  if (!GetProtocol()->Encode(message, &outbuf_)) {
    ERROR(logger, "AsyncClientConnect::EncodeMsg fail");
    return -2;
  }

  if (message->direction != EventMessage::kOneway) {
    if (0 != async_sequence_.Put(message)) {
      WARN(logger, "AsyncClientConnect::EncodeMsg Put fail");
      return -3;
    }
  }

  return 0;
}

void AsyncClientConnect::OnTimeout(EventMessage* msg){
  if (msg->type_id == MessageType::kHeartBeatMessage) {
    DEBUG(logger, "AsyncClientChannel::OnTimeout break");
    CleanClient();
  } else {
    DEBUG(logger, "AsyncClientChannel::OnTimeout message");
  }
  DoSendBack(msg, EventMessage::kStatusTimeout);
}

void AsyncClientConnect::CleanSequenceQueue(){
  std::list<EventMessage*> clear_list = async_sequence_.Clear();
  auto it = clear_list.begin();
  while (it != clear_list.end()) {
    EventMessage* msg = *it;
    it++;
    DoSendBack(msg, EventMessage::kInternalFailure);
  }
}

}

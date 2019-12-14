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
  TRACE(logger_, "OnDecodeMessage get sequence_id:" << message->sequence_id);
  if (nullptr == keeper_message) {
    DEBUG(logger, "OnDecodeMessage sequence not found or oneway msg:"
      <<*message << ", sequence_id:" << message->sequence_id
      <<",Sequence size:"<< async_sequence_.GetSequenceSize());
    MessageFactory::Destroy(message);
    return;
  }

  if (keeper_message->type_id == MessageType::kHeartBeatMessage) {
    TRACE(logger, "AsyncClientConnect::OnDecodeMessage heartbeat:" << *message);
    MessageFactory::Destroy(message);
    MessageFactory::Destroy(keeper_message);
    return;
  }

  message->birthtime = Time::GetCurrentClockTime();
  message->status = MessageBase::kStatusOK;
  message->direction = MessageBase::kIncomingResponse;
  message->sequence_id = keeper_message->sequence_id;
  message->coroutine_id = keeper_message->coroutine_id;
  message->handle_id = keeper_message->handle_id;
  message->descriptor_id = (int64_t)((void*)(this));
  if (nullptr != keeper_message->ctx && nullptr != keeper_message->ctx->callback){
    message->ctx = BDF_NEW(ContextBase);
    message->ctx->callback = keeper_message->ctx->callback;
    message->ctx->monitor_id = keeper_message->ctx->monitor_id;
  }

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

  TRACE(logger, "EncodeMsg msg type:" << (int)(message->type_id)
    << ",direction:" << (int)(message->direction));
  if (unlikely(!GetProtocol()->Encode(message, &outbuf_))) {
    ERROR(logger, "AsyncClientConnect::EncodeMsg fail");
    return -2;
  }

  //如果有应答需要将消息保存起来，kOnlySend属于无应答
  if (message->direction != EventMessage::kOnlySend) {
    TRACE(logger, "put sequence id:" << message->sequence_id);
    if (0 != async_sequence_.Put(message)) {
      WARN(logger, "AsyncClientConnect::EncodeMsg Put fail");
      return -3;
    }
  }

  return 0;
}

void AsyncClientConnect::OnTimeout(EventMessage* msg){
  if (msg->type_id == MessageType::kHeartBeatMessage) {
    //HeartBeat time out
    INFO(logger, "AsyncClientChannel::OnTimeout HeartBeat time out.");
    OnClose();
    //CleanClient();
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
    if (msg->timer_out_id > 0) {
      async_sequence_.CancelTimer(msg->timer_out_id);
    }
    DoSendBack(msg, EventMessage::kInternalFailure);
    it++;
  }
}

}

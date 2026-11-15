
#include "common/buffer.h"
#include "protocol/rapid/rapid_protocol.h"
#include "message.h"
#include <arpa/inet.h>

namespace bdf {

LOGGER_CLASS_IMPL(logger_, RapidProtocol);

struct RapidHead{
  static const uint16_t kMagic = 11111;
  static const uint16_t kMaxVersion = 10;
  // Heartbeat frame: version 1, cmd 255, empty body. Version 0 frames are
  // always business data, including cmd 255.
  static const uint8_t kHeartbeatVersion = 1;
  static const uint8_t kHeartbeatCmd = 255;

  uint16_t magic;
  uint8_t version;
  uint8_t cmd;
  uint32_t sqid_high;
  uint32_t sqid_low;
  uint32_t size;
};

void Int64To32(uint64_t value, uint32_t& high, uint32_t& low){
  low = value & 0xffffffff;
  high = value >> 32;
}

void Int32To64(uint32_t high, uint32_t low, uint64_t& value){
  value = ((uint64_t)(high) << 32) | low;
}

EventMessage* RapidProtocol::Decode(Buffer &input, bool& failed){
  size_t total = input.GetReadingSize();
  if (total < sizeof(RapidHead)){
    return nullptr;
  }

  char *data = input.GetReading();
  RapidHead *header = (RapidHead *)data;
  if (nullptr == header){
    WARN(logger_, "header is nullprt.");
    input.ResetAll();
    failed = true;
    return nullptr;
  }

  uint16_t magic_tmp = ntohs(header->magic);
  uint32_t sqid_high_tmp = ntohl(header->sqid_high);
  uint32_t sqid_low_tmp = ntohl(header->sqid_low);

  if (magic_tmp != RapidHead::kMagic
    || header->version > RapidHead::kMaxVersion){
    // data is not NUL-terminated: never stream it as a C string.
    WARN(logger_, "magic:" << magic_tmp << ",version:" << (int)header->version
      << ",input size:" << total);
    input.ResetAll();
    failed = true;
    return nullptr;
  }

  uint32_t size = ntohl(header->size);
  // Validate the declared length up front instead of buffering 30MB first.
  static const uint32_t kMaxFrameSize = 30 * 1024 * 1024;
  if (size < sizeof(RapidHead) || size > kMaxFrameSize){
    WARN(logger_, "invalid rapid frame size:" << size);
    failed = true;
    return nullptr;
  }
  if (total < size){
    // Incomplete frame is normal for TCP: wait for the rest, not an error.
    TRACE(logger_, "some bytes will decode next time,total:"<<total<<",size:"<<size);
    return nullptr;
  }

  RapidMessage *msg = MessageFactory::Allocate<RapidMessage>() ;
  msg->command = header->cmd;
  if (header->version == RapidHead::kHeartbeatVersion
    && header->cmd == RapidHead::kHeartbeatCmd && size == sizeof(RapidHead)) {
    msg->type_id = MessageType::kHeartBeatMessage;
  }
  msg->body.assign(data + sizeof(RapidHead), size - sizeof(RapidHead));
  Int32To64(sqid_high_tmp, sqid_low_tmp, msg->sequence_id);
  input.DrainReading(size);
  TRACE(logger_, "receive msg is:"<< msg->body<<",seqid:"<< msg->sequence_id);
  return msg;
}

bool RapidProtocol::Encode(EventMessage *pv, Buffer *output){
  RapidMessage *msg = nullptr;
  const bool heartbeat = pv->type_id == MessageType::kHeartBeatMessage;
  switch (pv->type_id) {
  case MessageType::kHeartBeatMessage: // HeartBeatRequest allocates a RapidMessage
  case MessageType::kRapidMessage:
    msg = dynamic_cast<RapidMessage *>(pv);
    break;
  default:
    ERROR(logger_, "unkown message type:"<<pv->type_id);
    return false;
  }

  if (msg == nullptr){
    WARN(logger_, "msg is nullptr.");
    return false;
  }

  if (!heartbeat && msg->body.empty() && msg->command == 0){
    WARN(logger_, "msg->body || cmd is empty");
    return false;
  }

  // TRACE(logger_, "send msg is:"<<msg->body<<",seqid:"<<msg->seqid);
  size_t length = heartbeat ? 0 : msg->body.length();
  size_t size = sizeof(RapidHead)+length;
  if (!output->EnsureSize(size)){
    WARN(logger_, "ensuresize error,size:"<<size);
    return false;
	}
  uint8_t *data = (uint8_t *)output->GetWriting();
  RapidHead *header = (RapidHead *)data;
  header->size = htonl(size);
  header->magic = htons(RapidHead::kMagic);
  uint32_t high = 0;
  uint32_t low = 0;
  Int64To32(msg->sequence_id, high, low);

  header->sqid_high = htonl(high);
  header->sqid_low = htonl(low);

  if (heartbeat) {
    header->version = RapidHead::kHeartbeatVersion;
    header->cmd = RapidHead::kHeartbeatCmd;
  } else {
    header->version = 0;
    header->cmd = msg->command;
  }
  data += sizeof(RapidHead);
  if (length && !heartbeat)
    memcpy(data, msg->body.c_str(), length);
  return output->PourWriting(size);

}

EventMessage* RapidProtocol::HeartBeatRequest(){
  // Encode used to reject this type, so Rapid heartbeats were never sent.
  RapidMessage* rapid_message = MessageFactory::Allocate<RapidMessage>();
  rapid_message->type_id = MessageType::kHeartBeatMessage;
  rapid_message->command = RapidHead::kHeartbeatCmd;
  rapid_message->direction = MessageBase::kOutgoingRequest;
  return rapid_message;
}

EventMessage* RapidProtocol::HeartBeatResponse(EventMessage* request){
  // Echo the frame (same sequence id) so the client matches it.
  request->direction = MessageBase::kOutgoingResponse;
  return request;
}

}

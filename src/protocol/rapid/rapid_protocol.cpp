
#include "common/buffer.h"
#include "protocol/rapid/rapid_protocol.h"
#include "message.h"
#include <arpa/inet.h>

namespace bdf {

LOGGER_CLASS_IMPL(logger_, RapidProtocol);

struct RapidHead{
  static const uint16_t kMagic = 11111;
  static const uint16_t kMaxVersion = 10;

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
    return NULL;
  }

  char *data = input.GetReading();
  RapidHead *header = (RapidHead *)data;
  if (NULL == header){
    input.ResetAll();
    failed = true;
    return NULL;
  }

  uint16_t magic_tmp = ntohs(header->magic);
  uint32_t sqid_high_tmp = ntohl(header->sqid_high);
  uint32_t sqid_low_tmp = ntohl(header->sqid_low);

  if (magic_tmp != RapidHead::kMagic
    || header->version > RapidHead::kMaxVersion){
    WARN(logger_, "magic:" << magic_tmp << ",version:" << header->version 
      << ",input size:" << total << ",input data:" << data);
    input.ResetAll();
    failed = true;
    return NULL;
  }

  uint32_t size = ntohl(header->size);
  if (total < size || size < sizeof(RapidHead)){
    TRACE(logger_, "some bytes will decode next time,total:"<<total<<",size:"<<size);
    failed = true;

    if (total >30* 1024 * 1024){
      input.DrainReading(total);
      WARN(logger_, "CustomProtocol DrainReading invalid data,total:" << total);
    }

    return NULL;
  }

  RapidMessage *msg = MessageFactory::Allocate<RapidMessage>() ;
  msg->command = header->cmd;
  msg->body.assign(data + sizeof(RapidHead), size - sizeof(RapidHead));
  Int32To64(sqid_high_tmp, sqid_low_tmp, msg->sequence_id);
  input.DrainReading(size);
  TRACE(logger_, "receive msg is:"<< msg->body<<",seqid:"<< msg->sequence_id);
  return msg;
}

bool RapidProtocol::Encode(EventMessage *pv, Buffer *output){
  RapidMessage *msg = NULL;
  switch (pv->type_id) {
  case MessageType::kRapidMessage:
    msg = dynamic_cast<RapidMessage *>(pv);
    break;
  default:
    ERROR(logger_, "unkown message type:"<<pv->type_id);
    return false;
  }

  if (msg == NULL){
    WARN(logger_, "msg is NULL.");
    return false;
  }

  if (msg->body.empty() && msg->command == 0){
    WARN(logger_, "msg->body & cmd is empty");
    return false;
  }

  // TRACE(logger_, "send msg is:"<<msg->body<<",seqid:"<<msg->seqid);
  size_t length = msg->body.length();
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

  header->version = 0;
  header->cmd = msg->command;
  data += sizeof(RapidHead);
  if (length)
    memcpy(data, msg->body.c_str(), length);
  return output->PourWriting(size);

}

}

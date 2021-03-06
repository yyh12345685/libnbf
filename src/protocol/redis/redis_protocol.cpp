
#include "protocol/redis/redis_protocol.h"
#include "protocol/redis/redis_parser.h"
#include "message.h"
#include "common/buffer.h"


namespace bdf {

LOGGER_CLASS_IMPL(logger_, RedisProtocol);

/// @remark maximal size allowed for buffering: 8M
static const uint32_t MaxSize_Allowed = 8 * 1024 * 1024;


EventMessage* RedisProtocol::Decode(Buffer &input, bool& failed){
  size_t size = input.GetReadingSize();
  if (0 == size){
    TRACE(logger_, "RedisProtocol::Decode input is empty!");
    return nullptr;
  }

  if (size > MaxSize_Allowed){
    TRACE(logger_, "RedisProtocol::Decode input is too large!");
    input.ResetAll();
    failed = false;
    return nullptr;
  }

  std::string reply(input.GetReading(), size);
  TRACE(logger_, "RedisProtocol::Decode input:" << reply);
  bool is_pong;
  size_t size_reply(0);
  std::vector<std::string> tmp_body;
  do {
    std::string result;
    size_t one_cmd_size(0);
    int ret = RedisParser::ParseReply(reply, &is_pong, &one_cmd_size, &result);
    if (0 != ret){
      if (ret > 0){
        return nullptr;
      }else{
        input.ResetAll();
        failed = false;
        WARN(logger_, "CodecRedis::Decode(): fail parse " << reply);
        return nullptr;
      }
    }
    size_reply += one_cmd_size;
    reply = reply.substr(one_cmd_size);
    tmp_body.push_back(result);
  } while (!reply.empty());

  input.DrainReading(size_reply);

  if (!is_pong){
    RedisMessage *msg = MessageFactory::Allocate<RedisMessage>();
    if (!msg){
      failed = true;
      return nullptr;
    }
    msg->bodys.swap(tmp_body);
    TRACE(logger_, "CodecRedis::Decode(): parse " << msg->bodys.size() 
      << ",seqid:" << msg->sequence_id);
    return msg;
  }
  return nullptr;
}

bool RedisProtocol::Encode(EventMessage *msg, Buffer *output){
  TRACE(logger_, "RedisProtocol::Encode start.");
  switch (msg->type_id){
  case MessageType::kRedisMessage:
  {
    RedisMessage *ev = static_cast<RedisMessage *>(msg);
    if (0 == ev->bodys.size()){
      INFO(logger_, "Redis 2 encode bodys is empty");
      return false;
    }

    std::string request;
    bool ret = true;
    std::string one_cmd;
    for (auto &it:ev->bodys){
      if (RedisParser::ParseCmd(it, one_cmd)){
        request.append(one_cmd);
      }else{
        ret = false;
        break;
      }
    }
    //TRACE(logger_, "RedisProtocol::Encode body:" << request);
    if (!ret || !output->EnsureSize(request.length())){
      WARN(logger_, "CodecRedis::Encode(): ret:" << ret 
        << ",fail parse:" << request<<",body size:"<< ev->bodys.size()<<",body[0]:"<< ev->bodys[0]);
      return false;
    }
    TRACE(logger_, "RedisProtocol::Encode(): parse result:" << request << ",size is:" << request.size());
    if (0 != request.length()){
      memcpy(output->GetWriting(), request.c_str(), request.length());
    }
    return output->PourWriting(request.length());
  }
  default:
    ERROR(logger_, "RedisProtocol::Encode unkown msg type:" << msg->type_id);
    return false;
  }
}

EventMessage* RedisProtocol::HeartBeatRequest() {
  RedisMessage* redis_message = MessageFactory::Allocate<RedisMessage>();
  redis_message->type_id = MessageType::kHeartBeatMessage;
  return redis_message;
}

EventMessage* RedisProtocol::HeartBeatResponse(EventMessage* request) {
  return request;
}

}
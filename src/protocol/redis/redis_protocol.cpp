
#include "protocol/redis/redis_protocol.h"
#include "protocol/redis/redis_parser.h"
#include "message.h"
#include "common/buffer.h"


namespace bdf {

LOGGER_CLASS_IMPL(logger_, RedisProtocol);

/// @remark maximal size allowed for buffering: 8M
static const uint32_t MaxSize_Allowed = 8 * 1024 * 1024;


EventMessage* RedisProtocol::Decode(Buffer &input, bool& failed){
  // A request that carried N commands gets its N replies back as ONE
  // message, in order. The connection matches one response per request, so
  // returning each reply separately shifted every later response by one.
  // Replies of the next request stay in the buffer for the next call.
  static const size_t kMaxLogLen = 64; // the buffer is not NUL-terminated
  while (true) {
    size_t size = input.GetReadingSize();
    if (0 == size){
      return nullptr;
    }
    const char* data = input.GetReading();
    TRACE(logger_, "RedisProtocol::Decode input size:" << size << ",head:"
      << std::string(data, size < kMaxLogLen ? size : kMaxLogLen));

    RedisReply reply;
    size_t size_reply(0);
    int ret = RedisParser::ParseReply(data, size, &size_reply, &reply);
    if (RedisParser::kParseNeedMore == ret){
      if (size > MaxSize_Allowed){
        // an incomplete reply bigger than the limit: give up on this
        // connection, dropping bytes would desync every following reply
        WARN(logger_, "RedisProtocol::Decode incomplete reply too large, size:" << size);
        failed = true;
      }
      return nullptr; // completed replies stay in partial_replies_
    }
    if (RedisParser::kParseOk != ret){
      WARN(logger_, "RedisProtocol::Decode fail parse, size:" << size << ",head:"
        << std::string(data, size < kMaxLogLen ? size : kMaxLogLen));
      failed = true;
      return nullptr;
    }
    input.DrainReading(size_reply);
    partial_replies_.push_back(std::move(reply));

    // An unsolicited reply (nothing outstanding) is delivered on its own.
    const size_t want = expected_replies_.empty() ? 1 : expected_replies_.front();
    if (partial_replies_.size() < want) {
      continue; // the rest of this request's replies may already be buffered
    }
    if (!expected_replies_.empty()) {
      expected_replies_.pop_front();
    }
    RedisMessage *msg = MessageFactory::Allocate<RedisMessage>();
    msg->replies.swap(partial_replies_);
    for (const auto& one : msg->replies) {
      one.Flatten(msg->bodys);
    }
    TRACE(logger_, "RedisProtocol::Decode(): replies:" << msg->replies.size());
    return msg;
  }
}

void RedisProtocol::Reset(){
  expected_replies_.clear();
  partial_replies_.clear();
}

bool RedisProtocol::Encode(EventMessage *msg, Buffer *output){
  TRACE(logger_, "RedisProtocol::Encode start.");
  switch (msg->type_id){
  case MessageType::kHeartBeatMessage:
  case MessageType::kRedisMessage:
  {
    // heartbeats created by HeartBeatRequest() are RedisMessage objects too
    RedisMessage *ev = dynamic_cast<RedisMessage *>(msg);
    if (nullptr == ev){
      ERROR(logger_, "RedisProtocol::Encode msg is not a RedisMessage, type:" << msg->type_id);
      return false;
    }
    if (ev->bodys.empty() && ev->cmds.empty()){
      INFO(logger_, "Redis 2 encode bodys is empty");
      return false;
    }

    std::string request;
    bool ret = true;
    std::string one_cmd;
    for (const auto& args : ev->cmds){
      if (!RedisParser::EncodeArgs(args, one_cmd)) {
        ret = false;
        break;
      }
      request.append(one_cmd);
    }
    for (auto &it:ev->bodys){
      if (!ret) break;
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
        << ",fail parse:" << request<<",body size:"<< ev->bodys.size()<<",cmds size:"<< ev->cmds.size());
      return false;
    }
    TRACE(logger_, "RedisProtocol::Encode(): parse result:" << request << ",size is:" << request.size());
    if (0 != request.length()){
      memcpy(output->GetWriting(), request.c_str(), request.length());
    }
    if (!output->PourWriting(request.length())) {
      return false;
    }
    expected_replies_.push_back(ev->cmds.size() + ev->bodys.size());
    return true;
  }
  default:
    ERROR(logger_, "RedisProtocol::Encode unkown msg type:" << msg->type_id);
    return false;
  }
}

EventMessage* RedisProtocol::HeartBeatRequest() {
  RedisMessage* redis_message = MessageFactory::Allocate<RedisMessage>();
  if (!redis_message){
    return nullptr;
  }
  redis_message->type_id = MessageType::kHeartBeatMessage;
  redis_message->bodys.push_back("PING");
  return redis_message;
}

EventMessage* RedisProtocol::HeartBeatResponse(EventMessage* request) {
  return request;
}

}
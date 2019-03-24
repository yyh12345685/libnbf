
#include "protocol/http/http_protocol.h"
#include "message_base.h"
#include "message.h"
#include "protocol/http/http_helper.h"
#include "common/buffer.h"
#include "common/string_util.h"

namespace bdf {

const unsigned char g_favicon_ico[] = {
  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x01,
  0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xde, 0x71, 0xeb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x11, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x10, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0xe0, 0x3f,
  0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x3f,
  0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0xff, 0xbf, 0x00, 0x00, 0xff, 0xbb,
  0x00, 0x00, 0xff, 0xb7, 0x00, 0x00, 0xff, 0xaf, 0x00, 0x00, 0xff, 0x9d, 0x00, 0x00, 0xff, 0xbb,
  0x00, 0x00, 0xff, 0xb7, 0x00, 0x00, 0xff, 0xaf, 0x00, 0x00, 0xff, 0x9f, 0x00, 0x00 };

const long c_favicon_ico = sizeof(g_favicon_ico);

LOGGER_CLASS_IMPL(logger_, HttpProtocol);

HttpProtocol::HttpProtocol(){
  agent_ = BDF_NEW(HttpAgent);
  agent_->parser_.InitParser(agent_);
}

HttpProtocol::~HttpProtocol() {
  BDF_DELETE(agent_);
}

EventMessage* HttpProtocol::Decode(Buffer &input, bool& failed){
	for(;;){
	  //not first
		if(agent_->msgs_.size()){
			HttpMessage* msg = agent_->msgs_.front();
			agent_->msgs_.pop_front();
      if (msg->http_info.url == "/favicon.ico"){
        volatile static uint64_t idx = 0;
        HeartBeatMessage* hmsg =  MessageFactory::Allocate<HeartBeatMessage>(); 
        hmsg->load_avg = __sync_add_and_fetch(&idx, 1);
        hmsg->heart_type = MessageType::kHttpMessage;
        hmsg->direction = msg->direction;
        MessageFactory::Destroy(msg);
        return hmsg;
      }
			return msg;
		}

	  //first
    int size = input.GetReadingSize();
    if (0 == size || failed) {
      //if paser failed
			TRACE(logger_,"not need parsed,size:"<<size<<",failed:"<<failed);
		  return nullptr;
	  }

    //handle size not bigger to 512k
    if (size > 512 * 1024)
      size = 512 * 1024;

    TRACE(logger_, "http decode size is: size:"<<size);
    TRACE(logger_, "http decode info is:"<<input.GetReading());
    int parsed = agent_->parser_.HttpParseRequest(input.GetReading(), size);
    if (agent_->parser_.GetHttpParser().upgrade){
      failed = true;
      ERROR(logger_, "upgrade is not supported yet");
      input.DrainReading(size);
      continue;
    }else if (parsed != size){
      enum http_errno err = (http_errno)agent_->parser_.GetHttpParser().http_errno;
      failed = true;
      ERROR(logger_, ",Read size is:" << size 
        << ",parsed is" << parsed << "input:" << input.GetReading());
      ERROR(logger_, "[" << http_errno_name(err)<< "]: " << http_errno_description(err)
        << ",msg size is:"<<agent_->msgs_.size());
      input.DrainReading(size);
      continue;
    }else{
      TRACE(logger_, "parased ok ,size:" << size);
    }
    
    //really this not need, but in test python,http_parser.cpp not to OnMessageComplete
    //so can reslove this bug,but not really good
    TRACE(logger_, "msg size is:" << agent_->msgs_.size() 
      << ",parsed body is:" << agent_->http_request_.body.size());
    if (agent_->http_request_.is_parsed_complete){
      input.DrainReading(size);
    }else{
      TRACE(logger_,"partly parsed.");
      //avoid unexcept data
      if (size > 128 *1024){
        input.DrainReading(size);
        WARN(logger_, "http DrainReading invalid data,size:"<<size);
      }else{
        TRACE(logger_, "may be not DrainReading,size:" << size);
      }
      failed = true;
    }
    //input.DrainReading(size);
	}
}

static void GetHeaderStr(const Dict& headers, std::string& str){
  str.clear();
  for (const auto& it:headers){
		std::string key(it.first.c_str(),it.first.size());
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    str.append(key.c_str(), key.size());
    str.append(": ");

    str.append(it.second.c_str(), it.second.size());
    str.append("\r\n");
  }
}

const char *ConvertStatusCode(int status, std::string& tmp) {
  switch (status) {
  case 200: return "200 OK";
  case 201: return "201 Created";
  case 202: return "202 Accepted";
  case 204: return "204 No Content";
  case 300: return "300 Multiple Choices";
  case 301: return "301 Moved Permanently";
  case 302: return "302 Moved Temporarily";
  case 304: return "304 Not Modified";
  case 400: return "400 Bad Request";
  case 401: return "401 Unauthorized";
  case 403: return "403 Forbidden";
  case 404: return "404 Not Found";
  case 405: return "405 Method Not Allowed";
  case 500: return "500 Internal Server Error";
  case 501: return "501 Not Implemented";
  case 502: return "502 Bad Gateway";
  case 503: return "503 Service Unavailable";
  default:
    tmp = common::ToString(status);
    return tmp.c_str();
  }
}

//HTTP_MSG
bool HttpProtocol::Encode(EventMessage *pv, Buffer *output){
  TRACE(logger_, "HttpProtocol::Encode.");
  HttpMessage *msg,one;

  switch (pv->type_id)
  {
  case MessageType::kHttpMessage:
    msg = static_cast<HttpMessage *>(pv);
    break;
  case MessageType::kHeartBeatMessage:{
    HeartBeatMessage *hmsg = static_cast<HeartBeatMessage *>(pv);
    msg = &one;
    msg->direction = hmsg->direction;
    if (msg->direction == MessageBase::kIncomingResponse) {
      msg->InitReply(nullptr, 200, false, true);
      msg->http_info.body.assign((const char *)g_favicon_ico, c_favicon_ico);
      msg->http_info.headers.insert(std::pair <std::string, std::string>("Content-Type", "image/x-icon"));
    }else {
      msg->InitRequest("GET", true, true, true);
      msg->http_info.url = "/favicon.ico";
    }
    msg->http_info.headers.insert(
      std::pair <std::string, std::string>("LoadAvg", common::ToString(hmsg->load_avg)));
    break;
  }
  default:
    ERROR(logger_, "unkown msg type:"<<pv->type_id);
    return false;
  }

  if (msg == nullptr){
    ERROR(logger_, "error ev == NULL.");
    return false;
  }

  if (msg->http_major != 1) return false;

  
  return WirteToBuf(msg, output);
}

bool HttpProtocol::WirteToBuf(HttpMessage *msg, Buffer *output){
  const char *http_version = nullptr;
  switch (msg->http_minor) {
  case 0:
    http_version = "HTTP/1.0";
    break;
  case 1:
    http_version = "HTTP/1.1";
    break;
  default:
    ERROR(logger_, "http_version is error,http_minor:" << msg->http_minor);
    return false;
  }
  std::string tmp;
  if (msg->direction == MessageBase::kIncomingResponse) {
    //as http server response 
    if (!output->Write(http_version, strlen(http_version))) return false;
    if (!output->Write(" ", 1)) return false;
    const char *status = ConvertStatusCode(msg->status_code, tmp);
    if (!output->Write(status, strlen(status))) return false;
  } else {
    //as http client request
    if (!output->Write(msg->http_info.method.c_str(), msg->http_info.method.size()))
      return false;
    if (!output->Write(" ", 1)) return false;
    if (!output->Write(msg->http_info.url.c_str(), msg->http_info.url.size()))
      return false;
    if (!output->Write(" ", 1)) return false;
    if (!output->Write(http_version, strlen(http_version))) return false;
  }
  if (!output->Write("\r\n", 2)) return false;

  if (msg->http_info.headers.size()) {
    std::string head_str;
    GetHeaderStr(msg->http_info.headers, head_str);
    if (!output->Write(head_str.c_str(), head_str.size())) return false;
  }

  size_t size = msg->http_info.body.size();
  if (!msg->GetHeader(("Content-Length"), NULL) && size > 0) {
    if (!output->Write("Content-Length", strlen("Content-Length"))) return false;
    if (!output->Write(": ", 2)) return false;
    tmp = common::ToString<uint64_t>(size);
    if (!output->Write(tmp.c_str(), tmp.size())) return false;
    if (!output->Write("\r\n", 2)) return false;
  }

  if (!msg->GetHeader(("Connection"), NULL)) {
    if (msg->keep_alive/* && msg->http_minor > 0*/) {
      if (!output->Write(
        "Connection: keep-alive\r\n", strlen("Connection: keep-alive\r\n"))) return false;
    } else {
      if (!output->Write(
        "Connection: close\r\n", strlen("Connection: close\r\n"))) return false;
    }
  }

  if (!output->Write("\r\n", 2)) return false;
  if (0 == size) return true;
  return output->Write(msg->http_info.body.c_str(), msg->http_info.body.size());
}

EventMessage* HttpProtocol::HeartBeatRequest() {
  HttpMessage* http_message = MessageFactory::Allocate<HttpMessage>();
  http_message->type_id = MessageType::kHeartBeatMessage;
  return http_message;
}

EventMessage* HttpProtocol::HeartBeatResponse(EventMessage* request) {
  return request;
}

}

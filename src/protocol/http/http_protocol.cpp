
#include <strings.h>
#include <algorithm>
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
  if (agent_){
    BDF_DELETE(agent_);
    agent_ = nullptr;
  }
}

void HttpProtocol::Reset(){
  for (HttpMessage* msg : agent_->msgs_) {
    MessageFactory::Destroy(msg);
  }
  agent_->msgs_.clear();
  agent_->pending_head_.clear();
  agent_->http_request_.ReSet();
  agent_->parser_.InitParser(agent_);
}

EventMessage* HttpProtocol::PopMessage(){
  if (agent_->msgs_.empty()) {
    return nullptr;
  }
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

EventMessage* HttpProtocol::OnEof(){
  // A response without Content-Length / chunking ends at EOF; http_parser
  // completes it only when told about EOF with a zero-length execute.
  agent_->parser_.HttpParseRequest(nullptr, 0);
  return PopMessage();
}

EventMessage* HttpProtocol::Decode(Buffer &input, bool& failed){
	for(;;){
	  //not first
		if(agent_->msgs_.size()){
			return PopMessage();
		}
	  //first
    int size = input.GetReadingSize();
    if (0 == size || failed) {
      //if paser failed
			TRACE(logger_,"not need parsed,size:"<<size<<",failed:"<<failed);
		  return nullptr;
	  }

    //handle size not bigger to 1M
    if (size > 1024 * 1024)
      size = 1024 * 1024;

    TRACE(logger_, "http decode size is: size:"<<size);
    // The buffer is not NUL-terminated; log a bounded copy only.
    TRACE(logger_, "http decode info is:" << std::string(input.GetReading(), std::min(size, 256)));
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
        << ",parsed is" << parsed << ",input:" << std::string(input.GetReading(), std::min(size, 256)));
      ERROR(logger_, "[" << http_errno_name(err)<< "]: " << http_errno_description(err)
        << ",msg size is:"<<agent_->msgs_.size());
      input.DrainReading(size);
      continue;
    }else{
      TRACE(logger_, "parased ok ,size:" << size);
    }
    
    // http_parser is stateful and has consumed these bytes, even when the
    // message is not complete yet. Drain them so they are not fed twice; an
    // incomplete message is not an error, the rest arrives with the next read.
    TRACE(logger_, "msg size is:" << agent_->msgs_.size()
      << ",parsed body is:" << agent_->http_request_.body.size());
    input.DrainReading(size);
	}
}

// No CR/LF/NUL: a header value or url containing them would split the
// message (header injection / response splitting / request smuggling).
static bool IsSafeText(const std::string& text){
  for (unsigned char ch : text) {
    if (ch == '\r' || ch == '\n' || ch == '\0') return false;
  }
  return true;
}

// RFC 7230 token: visible ASCII except separators.
static bool IsToken(const std::string& text){
  if (text.empty()) return false;
  for (unsigned char ch : text) {
    if (ch <= 32 || ch >= 127 || strchr("()<>@,;:\\\"/[]?={}", ch)) return false;
  }
  return true;
}

static bool HasHeader(const Dict& headers, const char* name){
  for (const auto& it : headers) {
    if (strcasecmp(it.first.c_str(), name) == 0) return true;
  }
  return false;
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
    // A status line needs a reason phrase; strict clients reject "418".
    tmp = common::ToString(status);
    tmp.append(" Unknown");
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
    if (msg->direction == MessageBase::kOutgoingResponse
      || msg->direction == MessageBase::kIncomingResponse) {
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
    ERROR(logger_, "error ev == nullptr.");
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
  const bool response = msg->direction == MessageBase::kOutgoingResponse
    || msg->direction == MessageBase::kIncomingResponse;
  for (const auto& it : msg->http_info.headers) {
    if (!IsToken(it.first) || !IsSafeText(it.second)) {
      ERROR(logger_, "invalid http header refused, name:" << it.first);
      return false;
    }
  }
  if (!response && (!IsToken(msg->http_info.method) || msg->http_info.url.empty()
      || !IsSafeText(msg->http_info.url)
      || msg->http_info.url.find(' ') != std::string::npos)) {
    ERROR(logger_, "invalid http request line refused, method:" << msg->http_info.method);
    return false;
  }
  std::string tmp;
  if (response) {
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
  const bool is_response = msg->direction == MessageBase::kOutgoingResponse
    || msg->direction == MessageBase::kIncomingResponse;
  // A response without a length is read until EOF, so a keep-alive reply
  // with an empty body never completed: send "Content-Length: 0". 1xx/204
  // must not carry it, and it must never be combined with Transfer-Encoding.
  const bool no_body_status = is_response
    && (msg->status_code / 100 == 1 || msg->status_code == 204);
  const bool need_length = is_response ? !no_body_status : size > 0;
  if (need_length
    && !HasHeader(msg->http_info.headers, "Content-Length")
    && !HasHeader(msg->http_info.headers, "Transfer-Encoding")) {
    if (!output->Write("Content-Length", strlen("Content-Length"))) return false;
    if (!output->Write(": ", 2)) return false;
    tmp = common::ToString<uint64_t>(size);
    if (!output->Write(tmp.c_str(), tmp.size())) return false;
    if (!output->Write("\r\n", 2)) return false;
  }

  if (!msg->GetHeader(("Connection"), nullptr)) {
    if (msg->keep_alive/* && msg->http_minor > 0*/) {
      if (!output->Write(
        "Connection: keep-alive\r\n", strlen("Connection: keep-alive\r\n"))) return false;
    } else {
      if (!output->Write(
        "Connection: close\r\n", strlen("Connection: close\r\n"))) return false;
    }
  }

  if (!output->Write("\r\n", 2)) return false;
  if (0 != size
    && !output->Write(msg->http_info.body.c_str(), msg->http_info.body.size())) {
    return false;
  }
  if (!response) {
    agent_->pending_head_.push_back(strcasecmp(msg->http_info.method.c_str(), "HEAD") == 0);
  }
  return true;
}

EventMessage* HttpProtocol::HeartBeatRequest() {
  // Encode() treats kHeartBeatMessage as HeartBeatMessage, so allocate that
  // type (an HttpMessage here was read through the wrong type).
  HeartBeatMessage* heartbeat = MessageFactory::Allocate<HeartBeatMessage>();
  heartbeat->direction = EventMessage::kOutgoingRequest;
  heartbeat->load_avg = 0;
  heartbeat->heart_type = MessageType::kHttpMessage;
  return heartbeat;
}

EventMessage* HttpProtocol::HeartBeatResponse(EventMessage* request) {
  // Decode produces a HeartBeatMessage; Encode writes the 200 reply itself.
  EventMessage* response = request;
  response->direction = EventMessage::kOutgoingResponse;

  return response;
}

}


#include <iostream>
#include <sstream>
#include <string.h>
#include <strings.h>

#include "protocol/http/http_helper.h"
#include "common/string_util.h"
#include "message.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, HttpParser);

void HttpParser::InitParser(HttpAgent *agent){
  http_parser_init(&parser_, HTTP_BOTH);

  parser_.data = agent;

  bzero(&settings_, sizeof(settings_));

  settings_.on_message_begin = OnMessageBegin;
  settings_.on_url = OnUrl;
  settings_.on_header_field = OnHeaderField;
  settings_.on_header_value = OnHeaderValue;
  settings_.on_headers_complete = OnHeadersComplete;
  settings_.on_body = OnBody;
  settings_.on_message_complete = OnMessageComplete;
}

int HttpParser::HttpParseRequest(const char* inbuf,int size){
  int drain = http_parser_execute(&parser_, &settings_, inbuf, size);

  if (HttpHasError()){
    WARN(logger_, "parse error,errno:" << parser_.http_errno << ",error info:" << GetHttpErrnoName());
    return -1;
  }

  return drain;
}

bool HttpParser::HttpHasError(){
  return parser_.http_errno != HPE_OK;
}

const char* HttpParser::GetHttpErrnoName(){
  return http_errno_name(HTTP_PARSER_ERRNO(&parser_));
}

//bool HttpParser::HttpBodyIsFinal(){
//  return http_body_is_final(&parser_);
//}

int HttpParser::OnMessageBegin(http_parser *parser){
  TRACE(logger_, "HttpParser::OnMessageBegin...");
  HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
  //reset avoid data is to long
  request_info.ReSet();
  request_info.is_parsed_complete = false;
  return 0;
}

int HttpParser::OnUrl(http_parser *parser, const char *at, size_t length){
	HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
  request_info.url.append(at, length);
  TRACE(logger_, "HttpParser::OnUrl:" << request_info.url);
  return 0;
}

int HttpParser::OnHeaderField(http_parser *parser, const char *at, size_t length){
	HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
  // A field may arrive in several callbacks when it spans two reads.
  if (request_info.parsing_header_value) {
    request_info.new_field.clear();
    request_info.parsing_header_value = false;
  }
  request_info.new_field.append(at, length);

  return 0;
}

int HttpParser::OnHeaderValue(http_parser *parser, const char *at, size_t length){
	HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
  std::string& value = request_info.headers[request_info.new_field];
  if (!request_info.parsing_header_value && !value.empty()) {
    // Repeated header line: join per RFC 7230 (Cookie uses "; ").
    value.append(strcasecmp(request_info.new_field.c_str(), "Cookie") == 0 ? "; " : ", ");
  }
  // A value may also arrive in several callbacks, so append.
  value.append(at, length);
  request_info.parsing_header_value = true;

  return 0;
}

int HttpParser::OnHeadersComplete(http_parser *parser){
  // This callback seems to have no meanning for us,
  // just reserved for future need.

  TRACE(logger_, "HttpParser::OnHeadersComplete...");
  return 0;
}

int HttpParser::OnBody(http_parser *parser, const char *at, size_t length){
	HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
    
  // NOTICE:OnBody may be called many times per Reuqest,
  // So not forget to use append but not assign :)
  // Bytes are no longer re-fed, so bound the body explicitly.
  static const size_t kMaxHttpBody = 30 * 1024 * 1024;
  if (request_info.body.size() + length > kMaxHttpBody) {
    WARN(logger_, "http body too large:" << request_info.body.size() + length);
    return -1; // makes http_parser fail -> connection is closed
  }
  request_info.body.append(at, length); 

  TRACE(logger_, "HttpParser::OnBody size is:" << length);
     
  return 0;
}

int HttpParser::OnMessageComplete(http_parser *parser){
	HttpAgent* agent = (HttpAgent*)parser->data;
  HttpRequestInfo& request_info = agent->http_request_;
  request_info.is_parsed_complete = true;

  TRACE(logger_, "http event,body:" << request_info.body 
    << ",body size:" << request_info.body.size()<<",type:"<< parser->type);
  
  HttpMessage *msg = MessageFactory::Allocate<HttpMessage>();
  msg->http_major = parser->http_major;
  msg->http_minor = parser->http_minor;
  msg->status_code = parser->status_code;
  msg->keep_alive = http_should_keep_alive(parser);

  if (parser->type == HTTP_REQUEST) {
    msg->direction = MessageBase::kIncomingRequest;
  } else {
    msg->direction = MessageBase::kIncomingResponse;
  }

	request_info.method = http_method_str((http_method)parser->method);
  msg->http_info.Swap(request_info);
  request_info.ReSet();

	agent->msgs_.push_back(msg);

	return 0;
}

LOGGER_CLASS_IMPL(logger_, HttpAgent);

HttpAgent::~HttpAgent(){
  TRACE(logger_, "should delete.....,http ev size:" << msgs_.size());

  for (const auto& msg : msgs_){
    TRACE(logger_, "should delete.....,http ev:"<<*msg);
    MessageFactory::Destroy(msg);
  }
}

}


#pragma once

#include <list>
#include "protocol/http/http_info.h"
#include "protocol/http/third/http_parser.h"
#include "common/logger.h"

namespace bdf {

class HttpMessage;

namespace protocol {

class HttpAgent;

//NOTICE:you must learn how to use http-parser first, or you may be confused:)
class HttpParser{
public:
  void InitParser(HttpAgent* agent);
  int HttpParseRequest(const char* inbuf, int size);

  //bool  HttpBodyIsFinal();

  static int OnMessageBegin(http_parser *parser);
  static int OnUrl(http_parser *parser, const char *at, size_t length);
  static int OnHeaderField(http_parser *parser, const char *at, size_t length);
  static int OnHeaderValue(http_parser *parser, const char *at, size_t length);
  static int OnHeadersComplete(http_parser *parser);
  static int OnBody(http_parser *parser, const char *at, size_t length);
  static int OnMessageComplete(http_parser *parser);

  http_parser& GetHttpParser(){ return parser_; }

private:
	LOGGER_CLASS_DECL(logger_);
  bool HttpHasError();
  const char* GetHttpErrnoName();

  http_parser          parser_;
  http_parser_settings settings_;
};

struct HttpAgent{
  HttpAgent(){}
  ~HttpAgent();
  HttpParser parser_;
	std::list<HttpMessage*> msgs_;
  HttpRequestInfo http_request_;

  LOGGER_CLASS_DECL(logger_);
};

}

}

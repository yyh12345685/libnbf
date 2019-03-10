
#pragma once

#include <string>
#include "protocol/http/http_info.h"
#include "common/logger.h"

namespace bdf {

struct HttpMessage;

class HttpUtil{
public:

static void HttpCookieDecode(const std::string &cookie, Dict& items);
//bool HttpCookieEncode(Dict& items, std::string *cookie);

static void BuildHttpRequest(HttpMessage* ev, HttpParsedInfo& info);

static void SplitHttpPostBody(HttpParsedInfo& info);

static void SplitToMap(const std::string& split_str, Dict& to_dict);

static int WrapHttpResponse(log4cplus::Logger& logger, HttpMessage* ev, HttpResponseInfo& response);

static std::string GetDomain(const std::string& url);

static std::string GetDomainPort(const std::string& url);

static std::string GetRootDomain(const std::string& domain);

static bool CheckRootDomainMatch(const std::string& base_str, const std::string& check_str);

};

}

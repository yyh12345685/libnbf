
#include "http_util.h"
#include "message.h"
#include "common/string_util.h"
#include <sstream>

namespace bdf {

static void GetUserId(HttpParsedInfo& info);

void HttpUtil::HttpCookieDecode(const std::string &cookie, Dict& items){
  std::vector<std::string> key_value;
  common::Split(key_value, cookie, ';');
  for (const auto& kv : key_value){
    size_t value_pos = kv.find("=");
    if (value_pos != std::string::npos){
      std::string key = kv.substr(0, value_pos);
      std::string value = kv.substr(value_pos + 1);
      common::Trim(key);
      common::Trim(value);

      items.insert(std::pair<std::string, std::string>(key, value));
    }
  }
}

//bool HttpCookieEncode(const Dict &items, std::string *cookie){
//  DictIter it = items.begin();
//  std::ostringstream oss;
//  for (int i = 0; it != items.end(); ++it, ++i) {
//    oss << (i == 0 ? "" : "; ") << it->first << "=" << it->second;
//  }
//  *cookie = oss.str();
//  return true;
//}

void HttpUtil::SplitToMap(const std::string& split_str, Dict& to_dict){
  std::vector<std::string> key_value;
  common::Split(key_value, split_str, '&');
  for (const auto& kv : key_value){
    size_t value_pos = kv.find("=");
    if (value_pos != std::string::npos){
      to_dict.insert(std::pair<std::string, std::string>
        ((common::UrlDecode(kv.substr(0, value_pos))), 
          common::UrlDecode(kv.substr(value_pos + 1), true)));
    }
  }
}

static void HandleTrackUrl(const std::string& split_str, Dict& to_dict)
{
  size_t pos = split_str.find("&click=");
  if (pos != std::string::npos){
    std::string new_split_str = split_str.substr(0, pos);
    to_dict.insert(std::pair<std::string, std::string>
      ("click", common::UrlDecode(split_str.substr(pos + 7), true)));
    HttpUtil::SplitToMap(new_split_str, to_dict);
  }else{
    HttpUtil::SplitToMap(split_str, to_dict);
  }
}

void HttpUtil::BuildHttpRequest(HttpMessage* ev, HttpParsedInfo& info){
  info.body = ev->http_info.body;
  info.url = ev->http_info.url;
  info.method = ev->http_info.method;

  for (const auto& it:ev->http_info.headers){
    if (it.first == "Cookie"){
      info.cookies_str = it.second;
    }
    else if (it.first == "X-Real-IP"){
      info.x_real_ip = it.second;
    }
    else if (it.first == "X-Forwarded-For"){
      info.x_forward_for = it.second;
    }
    info.headers.insert(it);
  }

  HttpCookieDecode(info.cookies_str, info.cookies);

  size_t first_pos = ev->http_info.url.find("http://");
  if (0 == first_pos){
    //start with http://
    first_pos = ev->http_info.url.find("/", first_pos+8);
    if(first_pos == std::string::npos)
      first_pos = 0;
  }else{
    first_pos = 0;
  }

  size_t param_pos = ev->http_info.url.find("?");
  if (param_pos == std::string::npos){
    param_pos = ev->http_info.url.find("/",1);
  }
  if (param_pos != std::string::npos){
    info.path = ev->http_info.url.substr(first_pos, param_pos-first_pos);
    std::string raw_param = ev->http_info.url.substr(param_pos + 1);
    if (info.path == "/ac" || info.path == "/as"
      || info.path == "/c" || info.path == "/w"){
      HandleTrackUrl(raw_param, info.url_param);
    }else{
      SplitToMap(raw_param, info.url_param);
    }
  }
  else{
    info.path = ev->http_info.url.substr(first_pos);
  }

  SplitHttpPostBody(info);

	GetUserId(info);
}

void HttpUtil::SplitHttpPostBody(HttpParsedInfo& info){
  if ((info.method == "POST" || info.method == "Post" || info.method == "post")
    && info.url_param.find("adxid") == info.url_param.end()/*not found adxid*/
    && info.path.find("madx") == std::string::npos/*not madx*/){
    //post request and not adx request
    SplitToMap(info.body, info.url_param);
  }
}

int HttpUtil::WrapHttpResponse(
  log4cplus::Logger& logger, HttpMessage* ev, HttpResponseInfo& response){

  if (-1 == response.status_code_){
    WARN(logger, "WrapHttpResponse status_code_ not set");
    return -1;
  }else{
    ev->status_code = response.status_code_;
  }
  ev->http_info.body += response.body_;

  for (const auto& it:response.headers_){
    ev->http_info.headers.insert(std::pair<std::string, std::string>(it.first, it.second));
  }

  return 0;
}

std::string HttpUtil::GetDomain(const std::string& url){
  const static std::string domain_begin("://");
  const static std::string domain_end_1("/");
  const static std::string domain_end_2(":");

  size_t begin_pos = url.find(domain_begin);
  if (begin_pos == std::string::npos) {
    begin_pos = 0;
  }
  else {
    begin_pos += domain_begin.size();
  }

  size_t end_pos_1 = url.find(domain_end_1, begin_pos);
  size_t end_pos_2 = url.find(domain_end_2, begin_pos);
  if (end_pos_1 == std::string::npos && end_pos_2 == std::string::npos) {
    return url.substr(begin_pos);
  }

  return url.substr(begin_pos, std::min(end_pos_1, end_pos_2) - begin_pos);
}

std::string HttpUtil::GetDomainPort(const std::string& url){
  const static std::string domain_begin("://");
  const static std::string domain_end("/");

  size_t begin_pos = url.find(domain_begin);
  if (begin_pos == std::string::npos){
    begin_pos = 0;
  }else{
    begin_pos += domain_begin.size();
  }

  size_t end_pos = url.find(domain_end, begin_pos);
  if (end_pos == std::string::npos){
    return url.substr(begin_pos);
  }

  return url.substr(begin_pos, end_pos - begin_pos);
}

//input mustbe domain
std::string HttpUtil::GetRootDomain(const std::string& domain){
  std::string real_domain = GetDomain(domain);
  std::string root_domain;
  std::vector<std::string> field_list;
  common::Split(field_list, real_domain, '.');
  if (field_list.size() >= 2){
    if (field_list.size() > 3){
      const std::string& field_section = field_list[field_list.size() - 2];
      if (field_section == "com"
        || field_section == "net"
        || field_section == "org"){
        root_domain = field_list[field_list.size() - 3];
        root_domain += ".";
      }
    }

    root_domain += field_list[field_list.size() - 2];
    root_domain += ".";
    root_domain += field_list[field_list.size() - 1];

    return root_domain;
  }

  return root_domain;
}

void GetUserId(HttpParsedInfo& info){
  //get http body  query string parameter uid:request_uid
  DictIter it = info.url_param.find("uid");
  if (it != info.url_param.end()){
    info.request_uid = it->second;
  }

  it = info.cookies.find("uid");
  if (it != info.cookies.end()){
    info.user_id_str = it->second;
  }
}

bool HttpUtil::CheckRootDomainMatch(const std::string& base_str, const std::string& check_str){
  std::vector<std::string> base_vc, check_vc;
  common::Split(base_vc, base_str, '.');
  common::Split(check_vc, check_str, '.');

  int base_idx = base_vc.size() - 1;
  int check_idx = check_vc.size() - 1;
  if (base_idx <= check_idx){
    for (; base_idx >= 0; --base_idx,--check_idx){
      if (base_vc[base_idx] != check_vc[check_idx]){
        return false;
      }
    }
    return true;
  }
  else{
    return false;
  }
}

}

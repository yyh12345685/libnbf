
#pragma once

#include <unordered_map>
#include <string>
#include <map>

namespace bdf {

typedef std::unordered_map<std::string, std::string> Dict;
typedef Dict::const_iterator DictIter;

typedef std::multimap<std::string, std::string> MultiDict;
typedef MultiDict::const_iterator MultiDictIter;

struct HttpRequestInfo{
  std::string method;    //GET, POST, HEADER for example.
  std::string url;

  Dict headers;
  std::string new_field; //field is waiting for value while parsing:)

  std::string body;

  bool is_parsed_complete;

  HttpRequestInfo() : is_parsed_complete(false){
  }

  void Swap(HttpRequestInfo& info){
    method.swap(info.method);
    url.swap(info.url);
    headers.swap(info.headers);
    body.swap(info.body);
    is_parsed_complete = info.is_parsed_complete;
  }

  void ReSet(){
    method.clear();
    url.clear();
    headers.clear();
    new_field.clear();
    body.clear();
  }

  void EraseHead(const std::string& key){
    const auto& xx = headers.find(key);
    if (xx != headers.end()){
      headers.erase(xx);
    }
  }
};

struct HttpParsedInfo{
	HttpParsedInfo(){
	}
  virtual ~HttpParsedInfo(){}

  std::string method;
  std::string url;
  Dict headers;
  Dict cookies;
  std::string cookies_str;
  std::string x_real_ip;
  std::string x_forward_for;
  std::string path;
	
  Dict url_param;

  std::string body;
  
	//
  std::string request_uid;	
  std::string user_id_str;
};

template<class Stream>
Stream& operator << (Stream &os, const HttpParsedInfo& http_parsed){
  os << "method: " << http_parsed.method;
  os << ",url: " << http_parsed.url;
  os << ",path: " << http_parsed.path << ",url_param:";

  for (const auto& it : http_parsed.url_param){
    os << it.first << "=" << it.second << "&";
  }

  os << ",headers: ";
  for (const auto& it : http_parsed.headers){
    os << it.first << "=" << it.second << ":";
  }

  os << ",cookies: ";
  for (const auto& it : http_parsed.cookies){
    os << it.first << "=" << it.second << ":";
  }

  os << ",cookie_str: " << http_parsed.cookies_str;
  os << ",request_uid: " << http_parsed.request_uid;
  os << ",user_id_str: " << http_parsed.user_id_str;
  os << ",body.size(): " << http_parsed.body.size();
  return os;
}

struct HttpResponseInfo{
  virtual ~HttpResponseInfo(){}
  enum {
    OK=200,
    NO_BIDDING = 204,
    LOCATION = 302,
    IFMODIFIEDSINCE = 304,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
  };

  HttpResponseInfo():status_code_(-1){}
  int status_code_;
  std::string body_;
  MultiDict headers_;
};

}
}


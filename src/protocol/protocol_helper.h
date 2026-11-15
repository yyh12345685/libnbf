
#pragma once

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "message_base.h"

namespace bdf{

class ProtocolHelper{
public:
  // "proto://host:port"; an IPv6 host must be bracketed: "rapid://[::1]:9091".
  static int ParseSpecAddr(const char *spec, char ip[256], int *port){
    if (!spec || strlen(spec) >= 256) {
      return MessageType::kUnknownEvent;
    }
    while (*spec == ' ') ++spec;
    const char* sep = strstr(spec, "://");
    if (!sep) return MessageType::kUnknownEvent;
    std::string proto(spec, sep - spec);
    const char* host = sep + 3;
    const char* host_end = nullptr;
    const char* colon = nullptr;
    if (*host == '[') {
      host_end = strchr(host, ']');
      if (!host_end || host_end[1] != ':') return MessageType::kUnknownEvent;
      colon = host_end + 1;
      ++host;
    } else {
      colon = strrchr(host, ':');
      if (!colon || memchr(host, ':', colon - host)) {
        return MessageType::kUnknownEvent; // bare IPv6 without brackets is ambiguous
      }
      host_end = colon;
    }
    size_t host_len = host_end - host;
    char* end = nullptr;
    long port_value = strtol(colon + 1, &end, 10);
    if (host_len == 0 || host_len >= 256 || end == colon + 1
      || port_value < 0 || port_value > 65535) {
      return MessageType::kUnknownEvent;
    }
    memcpy(ip, host, host_len);
    ip[host_len] = 0;
    *port = static_cast<int>(port_value);
    if (0 == strcasecmp("http", proto.c_str()))
      return MessageType::kHttpMessage;
    if (0 == strcasecmp("rapid", proto.c_str()))
      return MessageType::kRapidMessage;
    if (0 == strcasecmp("redis", proto.c_str()))
      return MessageType::kRedisMessage;
    return MessageType::kUnknownEvent;
  }

  static bool ParseDomainUrl(std::string& addr, std::string& domain, std::string& url){
    size_t domain_head = addr.find("://");
    if (domain_head == std::string::npos){
      //ip and domain must have http://
      return false;
    }
    domain_head += 3;

    size_t path_pos = addr.find("/", domain_head);
    size_t arg_pos = addr.find("?", domain_head);
    size_t domain_end = 0;
    if (path_pos != addr.npos){
      // find '/'
      domain_end = path_pos;
    } else if (arg_pos != addr.npos){
      //not find '/', find '?'
      domain_end = arg_pos;
    } else{
      //find nothing
      domain_end = addr.size();
    }

    domain = addr.substr(0, domain_end);

    if (path_pos != addr.npos){
      //find '/'
      url = addr.substr(path_pos);
    } else if (arg_pos != addr.npos){
      //not find '/', find '?'
      domain_end = arg_pos;
      url = '/' + addr.substr(arg_pos);
    }
    return true;
  }

  static void GetIpByDomain(const std::string& domain, std::vector<std::string>& ips){
    struct hostent *phost = gethostbyname(domain.c_str());
    if (nullptr == phost){
      return;
    }

    ips.clear();
    std::string ip;
    struct in_addr **addr_list = (struct in_addr **)phost->h_addr_list;
    for (int ix = 0; addr_list[ix] != nullptr; ix++){
      ip = inet_ntoa(*addr_list[ix]);
      ips.push_back(ip);
    }

  }

  static int ParseSpecDomain(const std::string& spec, std::vector<std::string>& ip_list, std::string& port){
    int domain_start_pos = 0;
    size_t pos = spec.find("://");
    if (pos != std::string::npos){
      domain_start_pos = pos + 3;
    }

    size_t end_pos = spec.find(":", domain_start_pos);
    if (end_pos == std::string::npos){
      port = "80";
      end_pos = spec.size();
    } else{
      port = spec.substr(end_pos + 1, (spec.size() - end_pos - 1));
    }

    std::string domain = spec.substr(domain_start_pos, (end_pos - domain_start_pos));

    GetIpByDomain(domain, ip_list);

    return MessageType::kHttpMessage;
  }
};

}

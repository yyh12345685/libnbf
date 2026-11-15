
#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <utility>
#include <errno.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common/logger.h"

namespace bdf {

class Socket{
 public:
  inline static bool IpToInt(const char* ipstr, uint32_t& ipint);
  static int CreateSocket(bool is_non_block=true,bool ipv6 = false); 
  static int Listen(const sockaddr_in& addr, size_t backlog = 2048,bool ipv6 = false);
  // addr may be IPv4 or IPv6 (unbracketed); the family follows the address,
  // the ipv6 argument is only kept for source compatibility.
  static int Listen(const char *addr, int port, size_t backlog = 2048,bool ipv6 = false);
  static int Listen(const sockaddr* addr, socklen_t len, size_t backlog);
  // Resolves host (numeric or, unless numeric_only, a DNS name; IPv4/IPv6).
  static bool ResolveAddr(const char* host, int port, bool passive, bool numeric_only,
    sockaddr_storage& out, socklen_t& len);

  struct ResolvedAddr {
    sockaddr_storage addr;
    socklen_t len;
  };
  // All addresses of host, IPv4 first (so a name that used to resolve to
  // IPv4 keeps doing so). May block on DNS: never call it on an I/O thread.
  static bool ResolveAll(const char* host, int port, std::vector<ResolvedAddr>& out);
  static int Accept(int listen_fd, char* ipbuf= nullptr, int* port= nullptr);

  static int Accept4(int listen_fd, char* ipbuf= nullptr, int* port= nullptr);
  /*
   * @return: <fd, whether need event>
   */
  static std::pair<int, bool> Connect(const char* ip,int port, bool is_non_block = true);
  static std::pair<int, bool> Connect(const sockaddr_in& addr, bool is_non_block=true);
  static std::pair<int, bool> Connect(const sockaddr* addr, socklen_t len, bool is_non_block=true);

  static bool SetNonBlock(int fd);
	static bool SetNoDelay(int fd, int enable);

  static bool SetSendTimeOut(int fd, int time_ms);
  static bool SetRecvTimeOut(int fd, int time_ms);

  static bool SetSoLinger(int fd, int seconds);

  static int TcpSocketRead(int fd, char *buf, int count) {
	  return read(fd, buf, count);
  }

	static int TcpSocketWrite(int fd, char *buf, int count) {
#ifdef MSG_NOSIGNAL
	  //suppress SIGPIPE errors, check EPIPE
	  int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif
		return send(fd, buf, count, flags);
	}

  static int WriteNonBlock(int fd, const void* buf, size_t count);
  static int ReadNonBlock(int fd, void* buf, size_t count);
  static int WriteVecNonBlock(int fd, iovec* iov, size_t& num_iov);

  static bool CheckConn(int fd);

  // Result of a finished nonblocking connect: 0 on success, else the errno.
  static int GetConnectError(int fd) {
    int error = 0;
    socklen_t length = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) != 0) {
      return errno;
    }
    return error;
  }

  inline static void Close(int fd);

  inline static int ShutDownBoth(int fd) {
    return shutdown(fd, SHUT_RDWR);
  }

  inline static bool IsIpAddr(const char* ip);

  static sockaddr_in GenerateAddr(const char *ip, const unsigned short port);

  static int SetCloseWait(int fd,int delay);
private:
	LOGGER_CLASS_DECL(logger_);
};

void Socket::Close(int fd) {
  close(fd);
}

bool Socket::IpToInt(const char* ipstr, uint32_t& ipint) {
  ipint=0;
  const char* cur=ipstr;
  for (size_t i=0; i<4; ++i) {
    int segvalue = atoi(cur);
    if ((segvalue>255) || (segvalue<0)) {
      return false;
    }

    ipint += segvalue * (uint32_t)(1 << ((3-i)*8));
    cur = strchr(cur, '.');
    if (3!=i && NULL==cur){
      return false;
    }
    cur+=1;
  }
  return true;
}

bool Socket::IsIpAddr(const char* ip){
  unsigned char buf[sizeof(struct in6_addr)];
  return inet_pton(AF_INET, ip, buf) == 1 || inet_pton(AF_INET6, ip, buf) == 1;
}

}


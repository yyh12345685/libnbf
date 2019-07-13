
#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common/logger.h"

namespace bdf {

class Socket{
 public:
  inline static bool IpToInt(const char* ipstr, uint32_t& ipint);
  static int CreateSocket(bool is_non_block=true); 
  static int Listen(const sockaddr_in& addr, size_t backlog = 2048);
  static int Listen(const char *addr, int port, size_t backlog = 2048);
  static int Accept(int listen_fd, char* ipbuf= nullptr, int* port= nullptr);

  /*
   * @return: <fd, whether need event>
   */
  static std::pair<int, bool> Connect(const char* ip,int port, bool is_non_block = true);
  static std::pair<int, bool> Connect(const sockaddr_in& addr, bool is_non_block=true);

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

  inline static void Close(int fd);

  inline static int ShutDownBoth(int fd) {
    return shutdown(fd, SHUT_RDWR);
  }

  inline static bool IsIpAddr(const char* ip);

  static sockaddr_in GenerateAddr(const char *ip, const unsigned short port);

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
  for (const char* ch = ip; *ch != '\0';++ch){
    if ((*ch <'0' || *ch >'9') && *ch != '.'){
      return false;
    }
  }
  return true;
}

}


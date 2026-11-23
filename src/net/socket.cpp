
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <netdb.h>
#include "net/socket.h"
#include "protocol/protocol_helper.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, Socket);

int Socket::CreateSocket(bool is_non_block,bool ipv6) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int ret, on;
  int fd = socket(domain, SOCK_STREAM,0);
  if (fd < 0 ){
		WARN(logger_, "creating socket: %s"<< strerror(errno));
    return -1;
  }

  if(is_non_block) {
    ret = SetNonBlock(fd);
    if(!ret){
      Close(fd);
		  WARN(logger_,"set no block O_NONBLOCK: %s" << strerror(errno));
      return -2;
    }
  }

  on=1;
  ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (0 != ret){
    Close(fd);
		WARN(logger_, "setsockopt SO_REUSEADDR: %s" << strerror(errno));
    return -3;
  }
	if(!SetNoDelay(fd,1)){
    Close(fd);
		WARN(logger_, "set nodelay error: %s" << strerror(errno));
    return -4;
	}

  ret = SetCloseWait(fd, 0);
  if (0 != ret){
		WARN(logger_, "SetCloseWait errno: %s" << strerror(errno));
    Close(fd);
    return -5;
  }
  return fd;

}

bool Socket::ResolveAddr(const char* host, int port, bool passive, bool numeric_only,
  sockaddr_storage& out, socklen_t& len){
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV | (passive ? AI_PASSIVE : 0)
    | (numeric_only ? AI_NUMERICHOST : 0);
  char service[16];
  snprintf(service, sizeof(service), "%d", port);
  struct addrinfo* result = nullptr;
  const char* node = (host && *host) ? host : nullptr;
  int ret = getaddrinfo(node, service, &hints, &result);
  if (ret != 0 || result == nullptr) {
    WARN(logger_, "getaddrinfo failed,host:" << (host ? host : "") << ",port:" << port
      << ",error:" << gai_strerror(ret));
    return false;
  }
  memcpy(&out, result->ai_addr, result->ai_addrlen);
  len = result->ai_addrlen;
  freeaddrinfo(result);
  return true;
}

bool Socket::ResolveAll(const char* host, int port, std::vector<ResolvedAddr>& out){
  out.clear();
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  char service[16];
  snprintf(service, sizeof(service), "%d", port);
  struct addrinfo* result = nullptr;
  int ret = getaddrinfo(host, service, &hints, &result);
  if (ret != 0 || result == nullptr) {
    WARN(logger_, "getaddrinfo failed,host:" << (host ? host : "") << ",port:" << port
      << ",error:" << gai_strerror(ret));
    return false;
  }
  for (int pass = 0; pass < 2; ++pass) {
    const int family = pass == 0 ? AF_INET : AF_INET6;
    for (struct addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
      if (ai->ai_family != family || ai->ai_addrlen > sizeof(sockaddr_storage)) continue;
      ResolvedAddr one;
      memcpy(&one.addr, ai->ai_addr, ai->ai_addrlen);
      one.len = ai->ai_addrlen;
      out.push_back(one);
    }
  }
  freeaddrinfo(result);
  return !out.empty();
}

int Socket::Listen(const char *addr, int port, size_t backlog,bool ipv6){
  (void)ipv6;
  sockaddr_storage sa;
  socklen_t len = 0;
  if (!ResolveAddr(addr, port, true, true, sa, len)) {
    return -1;
  }
  return Listen(reinterpret_cast<const sockaddr*>(&sa), len, backlog);
}

int Socket::Listen(const sockaddr_in& addr, size_t backlog,bool ipv6) {
  (void)ipv6;
  return Listen(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr), backlog);
}

int Socket::Listen(const sockaddr* addr, socklen_t len, size_t backlog) {
  int fd = CreateSocket(true, addr->sa_family == AF_INET6);
  if (fd < 0){
    return -2;
  }

  int ret = bind(fd, addr, len);
  if (0 != ret){
    WARN(logger_, "fd:"<<fd<<",bind: " << strerror(errno));
    Socket::Close(fd);
    return -3;
  }

  ret = listen(fd, backlog);
  if (0 != ret){
    WARN(logger_, "listen: " << strerror(errno));
    Socket::Close(fd);
    return -4;
  }
  return fd;
}

static void AddrToString(const sockaddr_storage& addr, char* ipbuf, int* port){
  if (addr.ss_family == AF_INET6) {
    const sockaddr_in6* v6 = reinterpret_cast<const sockaddr_in6*>(&addr);
    if (ipbuf) inet_ntop(AF_INET6, &v6->sin6_addr, ipbuf, INET6_ADDRSTRLEN);
    if (port) *port = ntohs(v6->sin6_port);
  } else {
    const sockaddr_in* v4 = reinterpret_cast<const sockaddr_in*>(&addr);
    if (ipbuf) inet_ntop(AF_INET, &v4->sin_addr, ipbuf, INET_ADDRSTRLEN);
    if (port) *port = ntohs(v4->sin_port);
  }
}

// Returns the fd, -1 when accept() failed (errno kept, e.g. EAGAIN/EMFILE),
// -2 when the accepted socket could not be set up (it is closed).
int Socket::Accept(int listen_fd, char* ipbuf, int* port){
  sockaddr_storage addr;
  socklen_t len_addr = sizeof(addr);
  int new_fd = -1;
  while (true) {
    len_addr = sizeof(addr);
    new_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&addr), &len_addr);
    if (-1==new_fd) {
      if (EINTR==errno) continue;
      return -1;
    }
    break;
  }

  if (!SetNonBlock(new_fd)) {
    Close(new_fd); // used to leak the fd
    return -2;
  }

  if(!SetNoDelay(new_fd,1)){
    WARN(logger_, "set nodelay error " << strerror(errno));
  }

  AddrToString(addr, ipbuf, port);
  return new_fd;
}

int Socket::Accept4(int listen_fd, char* ipbuf, int* port){
  int new_fd = -1;
  sockaddr_storage addr; // a plain sockaddr (16 bytes) truncated IPv6 peers
  socklen_t len = static_cast<socklen_t>(sizeof(addr));
  while (true) {
    len = sizeof(addr);
    new_fd = accept4(listen_fd, reinterpret_cast<sockaddr*>(&addr), &len,
      SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (new_fd < 0){
      if (errno == EINTR){
        continue;
      }
      return -1;
    }
    break;
  }
  AddrToString(addr, ipbuf, port);
  return new_fd;
}

int Socket::WriteNonBlock(int fd, const void* buf, size_t count) {
  size_t bytes_write=0;
  while (true) {
    ssize_t tmp_cnt = write(
      fd, reinterpret_cast<const char*>(buf)+bytes_write, count - bytes_write);
    if (tmp_cnt>0){
      bytes_write+=tmp_cnt;
      if (bytes_write==count){
        return count;
      } else if (bytes_write>count){
        return -1;
      }
    }else if (tmp_cnt<0) {
      if (EAGAIN==errno || EWOULDBLOCK==errno || EINTR==errno){
        return bytes_write;
      }else {
        return -2;
      }
    }else{
      return bytes_write;
    }
  }
}

int Socket::ReadNonBlock(int fd, void* buf, size_t count) {
  size_t bytes_read=0;
  while (true) {
    ssize_t tmp_cnt = read(fd, reinterpret_cast<char*>(buf)+bytes_read, count - bytes_read);
    if (tmp_cnt>0) {
      bytes_read+=tmp_cnt;
      if (bytes_read==count){
        return count;
      }else if (bytes_read>count){
        return -1;
      }
    }else if (tmp_cnt<0){
      if (EAGAIN==errno || EWOULDBLOCK==errno || EINTR==errno){
        return bytes_read;
      }else{
        return -2;
      }
    }else{
      return -3;
    }
  }
}

int Socket::WriteVecNonBlock(int fd, iovec* iov, size_t& num_iov){
  size_t bytes_write=0;
  while (true){
    ssize_t tmp_cnt = writev(fd, iov, num_iov);
    if (tmp_cnt>0){
      bytes_write+=tmp_cnt;
      size_t pos_iov;
      for (pos_iov=0; pos_iov<num_iov; ++pos_iov){
        if (tmp_cnt >= ssize_t(iov[pos_iov].iov_len)){
          tmp_cnt -= iov[pos_iov].iov_len;
        }else{
          break;
        }
      }

      num_iov=num_iov-pos_iov;
      if (0==num_iov){
        return bytes_write;
      }else{
        memmove(iov, iov + pos_iov, num_iov * sizeof(iovec));
        iov[0].iov_base = reinterpret_cast<char*>(iov[0].iov_base) + tmp_cnt;
        iov[0].iov_len -= tmp_cnt;
      }
    }else if (tmp_cnt<0){
      if (EAGAIN==errno || EWOULDBLOCK==errno || EINTR==errno){
        return bytes_write;
      }else{
        return -2;
      }
    }else{
      return bytes_write;
    }
  }
}

std::pair<int, bool> Socket::Connect(const char* ip, int port, bool is_non_block){
  sockaddr_storage sa;
  socklen_t len = 0;
  if (!ResolveAddr(ip, port, false, false, sa, len)) {
    return std::make_pair(-1, true);
  }
  return Connect(reinterpret_cast<const sockaddr*>(&sa), len, is_non_block);
}

std::pair<int, bool> Socket::Connect(const sockaddr_in& addr, bool is_non_block){
  return Connect(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr), is_non_block);
}

std::pair<int, bool> Socket::Connect(const sockaddr* addr, socklen_t len, bool is_non_block){
  int fd = CreateSocket(is_non_block, addr->sa_family == AF_INET6);
  if (fd < 0) return std::make_pair(-1, true);

  int ret = connect(fd, addr, len);
  if (0 == ret) {
    return std::make_pair(fd, true);
  }
  // After EINTR the connect continues asynchronously: wait for it like
  // EINPROGRESS (retrying connect() on the same fd would fail with EALREADY).
  if (EINPROGRESS == errno || EINTR == errno) {
    return std::make_pair(fd, false);
  }
  Socket::Close(fd);
  return std::make_pair(-1, true);
}

bool Socket::SetNonBlock(int fd) {
  int ret = fcntl(fd, F_GETFL, 0);
  if (-1 != ret){
    ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
    if (-1 != ret) return true;
  }
  return false;
}

bool Socket::SetNoDelay(int fd,int enable){
	int on = (enable ? 1 : 0);

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1){
		return false;
	}
	return true;
}

bool Socket::SetSendTimeOut(int fd, int time_ms){
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = time_ms * 1000;

  if (setsockopt(
    fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval)) == -1){
    return false;
  }
  return true;
}

bool Socket::SetRecvTimeOut(int fd, int time_ms){
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = time_ms * 1000;

  if (setsockopt(
    fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) == -1){
    return false;
  }

  return true;
}

bool Socket::SetSoLinger(int fd, int seconds){
  struct linger s_linger;

  s_linger.l_onoff = 1;
  s_linger.l_linger = seconds;

  if (setsockopt(
    fd, SOL_SOCKET, SO_LINGER,(const void *)&s_linger, sizeof(struct linger)) == -1){
    return false;
  }
  return true;
}

// IPv4 only; prefer ResolveAddr, which also handles IPv6.
sockaddr_in Socket::GenerateAddr(const char *ip, const unsigned short port) {
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  inet_aton(ip, &sa.sin_addr);
  return sa;
}

int Socket::SetCloseWait(int fd,int delay){
  struct linger ling;
  //在close socket调用后, 但是还有数据没发送完毕的时候容许逗留
  ling.l_onoff = 1;
  if(delay <=0 ){
    ling.l_linger = 0;//容许逗留的时间为0秒
  }else{
    ling.l_linger = delay;  //容许逗留的时间为delay秒
  }
  
  int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (&ling), (sizeof(linger)));
  return ret;
}

}

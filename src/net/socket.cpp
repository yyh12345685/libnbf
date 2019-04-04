
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include "net/socket.h"
#include "protocol/protocol_helper.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, Socket);

int Socket::CreateSocket(bool is_non_block) {
  int ret, on;
  int fd = socket(AF_INET, SOCK_STREAM,0);
  if (fd <= 0 ){
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
	}
  return fd;

}

int Socket::Listen(const char *addr, int port, size_t backlog){
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  if (addr && inet_aton(addr, &sa.sin_addr) == 0) {
    return -1;
  }
  return Listen(sa, backlog);
}

int Socket::Listen(const sockaddr_in& addr, size_t backlog) {
  int ret;
  int fd = CreateSocket();
  if (fd<0){
    return -2;
  }

  ret = bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
  if (0 != ret){
		WARN(logger_, "fd:"<<fd<<",bind: %s" << strerror(errno));
    if (fd>0){
      Socket::Close(fd);
    }
    return -3;
  }

  ret = listen(fd, backlog);
  if (0 != ret){
		WARN(logger_, "listen: %s" << strerror(errno));
    if (fd > 0){
      Socket::Close(fd);
    }
    return -4;
  }
  return fd;

}

int Socket::Accept(int listen_fd, char* ipbuf, int* port){
  sockaddr_in addr;
  socklen_t len_addr = sizeof(addr);
  int new_fd;
  while (true) {
    new_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&addr), &len_addr);
    if (-1==new_fd) {
      if (EINTR==errno) continue;
      return -1;
    }
    break;
  }

  bool ret = SetNonBlock(new_fd);
  if (!ret) return -2;

  if(!SetNoDelay(new_fd,1)){
		WARN(logger_, "set nodelay error %s" << strerror(errno));
	}

  if (nullptr !=ipbuf){
    inet_ntop(AF_INET, &addr.sin_addr, ipbuf, INET_ADDRSTRLEN);
  }

  if (nullptr !=port){
    *port = ntohs(addr.sin_port);
  }
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
        memcpy(iov, iov+pos_iov, num_iov * sizeof(iovec));
        iov[pos_iov].iov_base = reinterpret_cast<char*>(iov[pos_iov].iov_base) + tmp_cnt;
        iov[pos_iov].iov_len -= tmp_cnt;
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
  std::string final_ip;
  if (!IsIpAddr(ip)){
    //here is bug,TODO
    std::vector<std::string> ip_list;
    ProtocolHelper::GetIpByDomain(ip, ip_list);
    if (ip_list.size() > 0){
      final_ip = ip_list[0];
    }
  }else{
    final_ip.assign(ip);
  }
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  //if (ip && inet_aton(ip, &sa.sin_addr) == 0) {
  if (!final_ip.empty() && inet_aton(final_ip.c_str(),&sa.sin_addr)==0){
    return std::make_pair(-1, false);
  }
  return Connect(sa, is_non_block);
}

std::pair<int, bool> Socket::Connect(const sockaddr_in& addr, bool is_non_block){
  int fd = CreateSocket(is_non_block);
  if (fd <= 0) return std::make_pair(-1, true);

  int ret = connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
  if (0 == ret) {
    return std::make_pair(fd, true);
  }else if (EINPROGRESS == errno){
    return std::make_pair(fd, false);
  }else if (EINTR == errno){
    Socket::Close(fd);
    return Connect(addr, is_non_block);
  }else{
    Socket::Close(fd);
    return std::make_pair(-1, true);
  }
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

sockaddr_in Socket::GenerateAddr(const char *ip, const unsigned short port) {
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  inet_aton(ip, &sa.sin_addr);
  return sa;
}

}

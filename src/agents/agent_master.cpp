
#include "agents/agent_master.h"
#include "agents/agent_slave.h"
#include "agents/agents.h"
#include "protocol/protocol_helper.h"
#include "protocol/protocol_base.h"
#include "net/socket.h"
#include "app/config_info.h"
#include "event/event_driver.h"
#include "net/server_connect.h"
#include "net/connect_manager.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, AgentMaster);

AgentMaster::AgentMaster() :
  conf_(nullptr),
  release_mgr_(new ServerConnectDelayReleaseMgr()),
  idle_fd_(open("/dev/null", O_RDONLY | O_CLOEXEC)){
}

AgentMaster::~AgentMaster() {
  if (release_mgr_ != nullptr){
    delete release_mgr_;
  }
  close(idle_fd_);
}

bool AgentMaster::Init(const ServiceConfig* confs,const Agents* agents){
  conf_ = confs;
  agents_ = agents;

  for (const auto& addr:conf_->server_config){
    char ip[1024];
    int port;
    int cate = ProtocolHelper::ParseSpecAddr(addr.address.c_str(), ip, &port);
    if (cate == MessageType::kUnknownEvent) {
      WARN(logger_, "ParseSpecAddr faild,address:" << addr.address);
      continue;
    }

    int fd = Socket::Listen(ip, port, 20480);
    if (fd<0){
      WARN(logger_, "AgentMaster::Init SocketHelper::Listen error, ip:"
					<< ip <<"port:"<<port<<",fd,"<<fd << " errno:" << errno);
      continue;
    }

    if (0!=master_event_thread_.Add(fd, this) 
      || 0!= master_event_thread_.Modr(fd, true)){
      WARN(logger_, "AgentMaster::Init Add or Modr error, ip:"
					<< ip << "port:" << port <<",fd:"<<fd);
      Socket::Close(fd);
      continue;
    }
    
    std::pair<int, int> cate_port(cate, port);
    listened_fd_list_.insert(std::pair<int, std::pair<int, int>>(fd, cate_port));
  }

  if (listened_fd_list_.size() == 0){
    return false;
  }
  INFO(logger_, "listened_list_fd size:" << listened_fd_list_.size());

  return true;
}

void AgentMaster::OnEvent(EventDriver *poll, int fd, short event){
  const auto& listen_it = listened_fd_list_.find(fd);

  if (listen_it == listened_fd_list_.end()){
    WARN(logger_, "AgentMaster::OnEvent error fd event fd:" << fd );
    return;
  }

  //AcceptClient(poll,fd,listen_it->second);
  AcceptClient1(poll,fd,listen_it->second);
}

void AgentMaster::AcceptClient(
  EventDriver *poll, int fd,std::pair<int,int>& server_cate_port){
  int cate = server_cate_port.first;
  int listen_port = server_cate_port.second;
  char ip[256];
  int port;
  int sock = 0;
  while((sock = Socket::Accept(fd, ip, &port)) > 0){
    ServerConnect* svr_con = BDF_NEW(ServerConnect);
    std::string ip_str;
    ip_str.assign(ip, strlen(ip));
    svr_con->SetIp(ip_str);
    svr_con->SetFd(sock);
    svr_con->SetPort(port);
    svr_con->SetProtocol(cate);
    int register_thread_id = -1;
    if (0!= agents_->GetSlave()->AddModr(
      svr_con, sock, true,false,register_thread_id)){
      WARN(logger_, "AddModr faild,fd:"<<sock<<",reg tid:"<<register_thread_id);
      BDF_DELETE(svr_con);
      break;
    }
    release_mgr_->AddConnect(svr_con);
    poll->Wakeup();
    TRACE(logger_, "listen port:"<< listen_port <<",accept client ip:" << ip_str 
      << ",port:" << port << ",sock:" << sock << ",con's addr:" << svr_con
      << ",register_thread_id:"<<register_thread_id);
  }
  if(sock < 0 && errno == EMFILE){
      close(idle_fd_);
      int fd_tmp = Socket::Accept(fd);//优雅的关闭连接
      close(fd_tmp);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      INFO(logger_, "tmp fd:"<<fd_tmp<<",idle_file fd:"<< idle_fd_);
  }
}

void AgentMaster::AcceptClient1(
  EventDriver *poll, int fd,std::pair<int,int>& server_cate_port){
  //surport ipv6
  int cate = server_cate_port.first;
  int listen_port = server_cate_port.second;
  char ip[256];
  int port;
  int sock = 0;
  while(true){
    sock = Socket::Accept4(fd, ip, &port);
    if(sock>0){
      ServerConnect* svr_con = BDF_NEW(ServerConnect);
      std::string ip_str;
      ip_str.assign(ip, strlen(ip));
      svr_con->SetIp(ip_str);
      svr_con->SetFd(sock);
      svr_con->SetPort(port);
      svr_con->SetProtocol(cate);
      int register_thread_id = -1;
      if (0!= agents_->GetSlave()->AddModr(
        svr_con, sock, true,false,register_thread_id)){
        WARN(logger_, "AddModr faild,fd:"<<sock<<",reg tid:"<<register_thread_id);
        BDF_DELETE(svr_con);
        break;
      }
      release_mgr_->AddConnect(svr_con);
      poll->Wakeup();
      TRACE(logger_, "listen port:"<< listen_port <<",accept client ip:" << ip_str 
        << ",port:" << port << ",sock:" << sock << ",con's addr:" << svr_con
        << ",register_thread_id:"<<register_thread_id);
    }{
      if (errno == EMFILE) {
        close(idle_fd_);
        int fd_tmp = Socket::Accept4(fd);
        close(fd_tmp);
        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        INFO(logger_, "tmp fd:"<<fd_tmp<<",idle_file fd:"<< idle_fd_);
      } //else if (errno == EAGAIN)
      {
        break;
      }
    }
  }
}

bool AgentMaster::AreaseReleasedConnect(ServerConnect* con){
  return release_mgr_->SetRelease(con);
}

bool AgentMaster::Start() {
  int ret = master_event_thread_.Start();
  if (0!=ret) {
    return false;
  }
  return true;
}

void AgentMaster::Stop() {
  INFO(logger_, "AgentMaster::Stop.");
  master_event_thread_.Stop();
}

}


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
  conf_(nullptr){
}

AgentMaster::~AgentMaster() {
}

bool AgentMaster::Init(const IoServiceConfig* confs,const Agents* agents){
  conf_ = confs;
  agents_ = agents;

  for (const auto& addr:conf_->services_config){
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

  int cate = listen_it->second.first;
  int listen_port = listen_it->second.second;
  int sock = 0;
  char ip[256];
  int port;
	while((sock = Socket::Accept(fd, ip, &port)) > 0){
    ServerConnect* svr_con = BDF_NEW(ServerConnect);
    std::string ip_str;
    ip_str.assign(ip, strlen(ip));
    svr_con->SetIp(ip_str);
    svr_con->SetFd(sock);
    svr_con->SetPort(port);
    svr_con->SetProtocol(cate);
    svr_con->SetIsServer();
    if (0!= agents_->GetSlave()->AddModr(svr_con, sock, true)){
      WARN(logger_, "AddModr faild...");
      BDF_DELETE(svr_con);
      break;
    }
    if (ConnectManager::Instance().RegisterConnect((uint64_t)svr_con, svr_con)) {
      //for debug,难道是connect刚刚被释放了，新的connect来了，但是老的同样的指针地址还没有被移除注册
      Connecting* temp = ConnectManager::Instance().GetConnect((uint64_t)svr_con);
      INFO(logger_, "get prt:" << temp << ",set failed ptr:" << svr_con);
      //BDF_DELETE(svr_con);
      break;
    }
    
    TRACE(logger_, "listen port:"<< listen_port <<",accept client ip:" << ip_str 
      << ",port:" << port << ",sock:" << sock << ",con's addr:" << svr_con);
  }
}

bool AgentMaster::Start() {
  int ret = master_event_thread_.Start();
  if (0!=ret) {
    return false;
  }
  return true;
}

void AgentMaster::Stop() {
  TRACE(logger_, "AgentMaster::Stop.");
  master_event_thread_.Stop();
}

}


#include "agent_master.h"
#include "agents.h"
#include "agent_slave.h"
#include "protocol/protocol_helper.h"
#include "net/socket.h"
//#include "app.h"
//#include "connecting.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, AgentMaster);

AgentMaster::AgentMaster() :
  conf_(NULL),
  agents_(NULL){
}

bool AgentMaster::Init(const AppCofing* confs, const Agents* agents){
  conf_ = confs;
  agents_ = agents;

  EventDriver* events_driver = master_event_thread_.GetPoll();
  for (const auto& addr:conf_->listener_){
    char ip[1024];
    int port;
    int cate = protocol::ProtocolHelper::ParseSpecAddr(addr.c_str(), ip, &port);

    int fd = net::Socket::Listen(ip, port, 20480);
    if (fd<0){
      WARN(logger_, "AgentMaster::Init SocketHelper::Listen error, ip:"
					<< ip <<"port:"<<port<<",fd,"<<fd << " errno:" << errno);
      continue;
    }

    ListenInfo info;
    info.cate = cate;
    info.addr = addr;

    if (events_driver->Add(fd, this) == -1){
      WARN(logger_, "AgentMaster::Init events_driver->Add error, ip:"
					<< ip << "port:" << port <<",fd:"<<fd);
      net::Socket::Close(fd);
      continue;;
    }
    events_driver->Modr(fd, true);
    listened_list_.insert(std::pair<int, ListenInfo>(fd, info));
  }

  if (listened_list_.size() == 0){
    return false;
  }

  for (const auto& fd_tmp : listened_list_){
    events_driver->AddListenedListFd(fd_tmp.first);
    TRACE(logger_, "AddListenedListFd:" << fd_tmp.first);
  }
  std::unordered_set<int>&listened_list_fd_tmp = events_driver->GetListenedListFd();
  INFO(logger_, "listened_list_fd size:" << listened_list_fd_tmp.size());

  return true;
}

void AgentMaster::OnEvent(EventDriver *poll, int fd, short event){
  const auto& listen_it = listened_list_.find(fd);

  if (listen_it == listened_list_.end()){
    WARN(logger_, "AgentMaster::OnEvent error fd event fd:" << fd );
    return;
  }

  int sock = 0;
  char ip[256];
  int port;
	while((sock = net::Socket::Accept(fd, ip, &port)) > 0){
  //  std::pair<uint64_t, EpollSpecificThread *> pair = pool.ChooseEpollSpecificThread();
		//std::tr1::shared_ptr<HandlerMessageBase> handle = agents_->GetHandles()[pair.first];
  //  Connecting* connect = new Connecting(sock, pair.second->GetPoll(), handle);

  //  std::string ip_str;
  //  ip_str.assign(ip, strlen(ip));
  //  connect->SetIp(ip_str);
  //  connect->SetPort(port);
  //  connect->SetMaxSendBufFailedSize(conf_->max_send_buf_failed_size_);
  //  connect->SetProtocolCate(listen_it->second.cate);
  //  connect->SetClosedMgr(this);
  //  connect->SetIsServer(true);
  //  con_release_.AddConnect(connect);
  //  connect->RegisterAaccept(sock, true);
  //  pair.second->GetPoll()->Wakeup();

    TRACE(logger_, "accept client ip:" << ip_str << ",port:" << port 
      << ",sock:" << sock << ",con's addr:" << connect << ",con seqid" << connect->GetSequencyId());
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
  master_event_thread_.Stop();
}

AgentMaster::~AgentMaster(){
  Stop();
}

}

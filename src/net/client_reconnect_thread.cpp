
#include <unistd.h>
#include <functional>
#include "net/client_reconnect_thread.h"

namespace bdf{

bool ClientReconnect::StartThread(){
  is_running_ = true;
  client_thread_ = new std::thread(std::bind(&ClientReconnect::Run, this));
  return true;
}

void ClientReconnect::StopThread(){
  is_running_ = false;
  if (nullptr != client_thread_){
    client_thread_->join();
    delete client_thread_;
    client_thread_ = nullptr;
  }
}

void ClientReconnect::Run(){
  while (is_running_){
    timer_.ProcessTimer();
    usleep(1000);
  }
}

}

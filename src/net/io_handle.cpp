#include <unistd.h>
#include <sys/prctl.h>
#include "net/io_handle.h"
#include "message_base.h"
#include "handle_data.h"
#include "net/connect.h"
#include "service/io_service.h"
#include "net/connect_manager.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger, IoHandler);

thread_local IoHandler* IoHandler::io_handler_ = nullptr;

void IoHandler::Run(HandleData* data){
  prctl(PR_SET_NAME, "IoHandler");
  TRACE(logger, "IoHandler::Run start.");
  io_handler_ = this;
  time_t now = time(NULL);
  while (data->is_run) {
    GetTimer().ProcessTimer();
    if (data->data_.empty()) {
      usleep(1);
      continue;
    }

    std::queue<EventMessage*> temp;
    data->lock_.lock();
    temp.swap(data->data_);
    data->lock_.unlock();

    while (!temp.empty()) {
      EventMessage *msg = temp.front();
      temp.pop();
      Handle(msg);
      //for debug
      time_t cur_time = time(NULL);
      if ((cur_time - now) > 300) {
        INFO(logger, "msg size:" << data->data_.size()
          << ",handle size:" << temp.size());
        now = cur_time;
      }
    }
  }
  TRACE(logger, "IoHandler::Run exit.");
}

void IoHandler::Handle(EventMessage* message){
  TRACE(logger, "IoHandler::Handle msg:" << *message);

  switch (message->type_io_event) {
    case MessageType::kIoMessageEvent:
    HandleIoMessageEvent(message);
    break;
  case MessageType::kIoActiveCloseEvent:
    HandleIoActiveCloseEvent(message);
    break;
  default:
    WARN(logger, "IoHandler unknown message:" << message->type_id);
    break;
  }
}

void IoHandler::HandleIoMessageEvent(EventMessage* message){
  TRACE(logger, "HandleIoMessageEvent");
  Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  if (!reg_con){
    INFO(logger, "HandleIoMessageEventt reg_con is closed.");
    HandleIoMessageFailed(message);
    return;
  }

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con || reg_con != con) {
    ERROR(logger, "error,con:"<< con <<",reg_con:"<< con);
    HandleIoMessageFailed(message);
    return;
  }

  if (0 != con->EncodeMsg(message)){
    HandleIoMessageFailed(message);
    return;
  }
  //单向消息encode之后即可释放，双向消息会在返回或者超时的时候释放
  //server答复的消息需要在这里释放掉
  if (message->direction == EventMessage::kOnlySend
    || message->direction == EventMessage::kOutgoingResponse) {
    MessageFactory::Destroy(message);
  }

  con->OnWrite();
}

void IoHandler::HandleIoMessageFailed(EventMessage* message) {
  if (message->direction == EventMessage::kOutgoingRequest) {
    message->status = EventMessage::kInternalFailure;
    IoService::GetInstance().SendToServiceHandle(message);
  } else {
    MessageFactory::Destroy(message);
  }
}

void IoHandler::HandleIoActiveCloseEvent(EventMessage* message){
  TRACE(logger, "HandleIoActiveCloseEvent");
  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger, "HandleIoActiveCloseEvent descriptor is not Connecting");
    return;
  }

  MessageFactory::Destroy(message);
  con->OnActiveClose();
  con->Destroy();
}

}

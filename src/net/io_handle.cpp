#include <unistd.h>
#include <sys/prctl.h>
#include "net/io_handle.h"
#include "message_base.h"
#include "handle_data.h"
#include "net/connect.h"
#include "service/io_service.h"
#include "net/connect_manager.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, IoHandler);

thread_local IoHandler* IoHandler::io_handler_ = nullptr;

void IoHandler::Run(HandleData* data){
  prctl(PR_SET_NAME, "IoHandler");
  INFO(logger_, "IoHandler::Run start.");
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
        INFO(logger_, "msg size:" << data->data_.size()
          << ",handle size:" << temp.size());
        now = cur_time;
      }
    }
  }
  INFO(logger_, "IoHandler::Run exit.");
}

void IoHandler::Handle(EventMessage* message){
  TRACE(logger_, "IoHandler::Handle msg:" << *message);
  switch (message->type_io_event) {
  case MessageType::kIoEvent:{
    if (message->event_mask & EVENT_READ) {
      HandleReadEvent(message);
    }
    if (message->event_mask & EVENT_WRITE) {
      HandleWriteEvent(message);
    }
    if ((message->event_mask & EVENT_CONNECT_CLOSED) 
      || (message->event_mask & EVENT_ERROR)) {
      HandleCloseEvent(message);
    }
    MessageFactory::Destroy(message);
    break; 
  }
  case MessageType::kIoHandleEventMsg:
    HandleIoMessageEvent(message);
    break;
  case MessageType::kIoActiveCloseEvent:
    HandleIoActiveCloseEvent(message);
    break;
  default:
    MessageFactory::Destroy(message);
    WARN(logger_, "IoHandler unknown message:" << message->type_id);
    break;
  }
}

void IoHandler::HandleIoMessageEvent(EventMessage* message){
  TRACE(logger_, "HandleIoMessageEvent");
  //Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  //if (!reg_con){
  //  if (1 == rand() % 300)
  //    INFO(logger_, "con is closed,ptr:"<< con);
  //  HandleIoMessageFailed(message);
  //  return;
  //}
  
  if (!con /*|| reg_con != con*/) {
    ERROR(logger_, "error,con:"<< con /*<<",reg_con:"<< reg_con*/);
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

void IoHandler::HandleReadEvent(EventMessage* message){
  TRACE(logger_, "HandleReadEvent");
  //Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  //if (!reg_con) {
  //  INFO(logger_, "HandleReadEvent reg_con is closed,ptr:" << con);
  //  return;
  //}

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger_, "HandleReadEvent descriptor is not Connecting");
    return;
  }
  con->OnRead();
}

void IoHandler::HandleWriteEvent(EventMessage* message){
  TRACE(logger_, "HandleWriteEvent");
  //Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  //if (!reg_con) {
  //  INFO(logger_, "WriteEvent reg_con is closed,ptr:" << con);
  //  return;
  //}

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger_, "HandleWriteEvent descriptor is not Connecting");
    return;
  }
  con->OnWrite();
}

void IoHandler::HandleCloseEvent(EventMessage* message){
  TRACE(logger_, "HandleCloseEvent");
  //Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  //if (!reg_con) {
  //  INFO(logger_, "CloseEvent reg_con is closed,ptr:" << con);
  //  return;
  //}

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    WARN(logger_, "HandleCloseEvent descriptor is not Connecting");
    return;
  }
  con->OnClose();
}

void IoHandler::HandleIoActiveCloseEvent(EventMessage* message){
  TRACE(logger_, "HandleIoActiveCloseEvent");
  //Connecting* reg_con = ConnectManager::Instance().GetConnect(message->descriptor_id);
  //if (!reg_con) {
  //  INFO(logger_, "HandleIoActiveCloseEvent reg_con is closed.");
  //  HandleIoMessageFailed(message);
  //  return;
  //}

  Connecting* con = (Connecting*)((void*)(message->descriptor_id));
  if (!con) {
    MessageFactory::Destroy(message);
    WARN(logger_, "HandleIoActiveCloseEvent descriptor is not Connecting");
    return;
  }

  MessageFactory::Destroy(message);
  con->Destroy();
}

}

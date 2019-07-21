
#include <functional>
#include <unistd.h>
#include <sys/prctl.h>
#include "common/thread_id.h"
#include "service/coroutine_service_handle.h"
#include "coroutine/coroutine_context.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_actor.h"
#include "handle_data.h"
#include "task.h"
#include "net/connect.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineServiceHandler);

void CoroutineServiceHandler::Run(HandleData* data){
  prctl(PR_SET_NAME, "CoroutineServiceHandler");
  TRACE(logger_, "CoroutineServiceHandler::Run.");
  CoroutineContext::Instance().Init(this,&timer_);
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  scheduler->InitCoroSchedule(&CoroutineServiceHandler::ProcessCoroutine, data);

  while (data->is_run) {
    ProcessTimer();
    ProcessTask(data);

    if (data->data_.empty()) {
      usleep(1);
      continue;
    }
    std::queue<EventMessage*> temp;
    data->lock_.lock();
    temp.swap(data->data_);
    data->lock_.unlock();
    ProcessMessageHandle(temp);
  }
  INFO(logger_, "CoroutineServiceHandler::Run will be exit.");
}

void CoroutineServiceHandler::ProcessCoroutine(void* data){
  TRACE(logger_, "CoroutineServiceHandler::Process. thread id:"<< ThreadId::Get());
  HandleData* handle_data = (HandleData*)data;
  CoroutineServiceHandler* handle = 
    dynamic_cast<CoroutineServiceHandler*>(handle_data->handle_);
  if (handle == nullptr){
    ERROR(logger_, "handle is null prt...");
    return;
  }

  while (handle_data->is_run) {
    //handle->ProcessTimer();
    //handle->ProcessTask(handle_data);
    handle->Process(handle_data);
  }
  TRACE(logger_, "CoroutineServiceHandler thread will be exit.");
}

void CoroutineServiceHandler::ProcessTimer() {
  timer_.ProcessTimer();
}

//when send receive timeout
void CoroutineServiceHandler::OnTimer(void* function_data){
  TRACE(logger_, "CoroutineServiceHandler::OnTimer.");
  int coro_id = *(int*)function_data;
  if (coro_id < 0) {
    //not to here,否则会丢消息
    ERROR(logger_, "error coro_id:" << coro_id);
    return;
  }
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  scheduler->ProcessCoroutine(coro_id);
}

void CoroutineServiceHandler::ProcessTask(HandleData* data){
  if (data->task_.empty()){
    return;
  }

  TRACE(logger_, "CoroutineServiceHandler::ProcessTask.");
  std::queue<Task*> temp;
  temp.swap(data->task_);

  while (!temp.empty()) {
    Task *task = temp.front();
    temp.pop();
    task->OnTask(nullptr);
  }

}

void CoroutineServiceHandler::Process(HandleData* data){
  if (data->data_.empty()) {
    usleep(1);
    return;
  }

  std::queue<EventMessage*> temp;
  data->lock_.lock();
  temp.swap(data->data_);
  data->lock_.unlock();

  ProcessMessageHandle(temp);
}

void CoroutineServiceHandler::ProcessClientItem(EventMessage* msg){
  if (msg->coro_id<0){
    //not to here,否则会丢消息
    ERROR(logger_, "error coro_id:" << msg->coro_id);
    MessageFactory::Destroy(msg);
    return;
  }
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  CoroutineActor* coroutine = scheduler->GetCoroutineCtx(msg->coro_id);
  coroutine->SendMessage(msg);
  scheduler->ProcessCoroutine(msg->coro_id);
}

void CoroutineServiceHandler::ProcessMessageHandle(std::queue<EventMessage*>& msg_list) {
  TRACE(logger_, "handle id:" << GetHandlerId()
    << ",ProcessMessage size:" << msg_list.size());
  while (!msg_list.empty()) {
    EventMessage *msg = msg_list.front();
    msg_list.pop();

    Connecting* con = (Connecting*)((void*)(msg->descriptor_id));
    TRACE(logger_, "ProcessMessageHandle is server:" << con->IsServer());

    if (con->IsServer()){
      //处理服务端接收的消息
      Handle(msg);
    }else{
      //处理客户端接收的消息
      ProcessClientItem(msg);
    }
  }
}

}

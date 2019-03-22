
#include <functional>
#include <unistd.h>

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
  CoroutineContext::Instance().Init(this,&timer_);

  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  CoroutineActor* coroutine = CoroutineContext::Instance().GetCoroutine();
  CoroutineFunc func = &CoroutineServiceHandler::Process;
  current_coroutine_id_ = scheduler->CoroutineNew(coroutine, func, data);
  while (scheduler->CoroutineStatus(coroutine, current_coroutine_id_)) {
    scheduler->CoroutineResume(coroutine, current_coroutine_id_);
  }
}

void CoroutineServiceHandler::Process(CoroutineActor* coroutine, void* data){
  HandleData* handle_data = (HandleData*)data;
  CoroutineServiceHandler* handle = dynamic_cast<CoroutineServiceHandler*>(handle_data->handle_);
  if (handle == nullptr){
    ERROR(logger_, "handle is null prt...");
    return;
  }
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  while (handle_data->is_run) {
    handle->ProcessTimer();
    handle->ProcessTask(handle_data);
    handle->Process(handle_data);
    scheduler->CoroutineYield(coroutine);
  }
}

void CoroutineServiceHandler::ProcessTimer() {
  timer_.ProcessTimer();
}

void CoroutineServiceHandler::OnTimer(void* function_data){

}

void CoroutineServiceHandler::ProcessTask(HandleData* data){
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
    return;
  }

  std::queue<EventMessage*> temp;
  data->lock_.lock();
  temp.swap(data->data_);
  data->lock_.unlock();

  ProcessMessage(temp);
}

void CoroutineServiceHandler::ProcessMessage(std::queue<EventMessage*>& msg_list){
  while (!msg_list.empty()) {
    EventMessage *msg = msg_list.front();
    msg_list.pop();
    Connecting* con = (Connecting*)((void*)(msg->descriptor_id));

    if (con->GetIsServer()){
      //处理服务端接收的消息
    } else {
      //处理客户端接收的消息
      CoroutineActor* coroutine = CoroutineContext::Instance().GetCoroutine();
      coroutine->SendMessage(msg);
    }

    Handle(msg);
  }
}

}


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
  CoroutineActor* coroutine = CoroutineContext::Instance().GetCoroutine();
  CoroutineFunc func = &CoroutineServiceHandler::ProcessCoroutine;
  current_coroutine_id_ = scheduler->CoroutineNew(coroutine, func, data);
  //current_timer_coroutine_id_ = scheduler->CoroutineNew(coroutine, func, data);
  //current_task_coroutine_id_ = scheduler->CoroutineNew(coroutine, func, data);
  if(scheduler->CoroutineStatus(coroutine, current_coroutine_id_)/*&&
    scheduler->CoroutineStatus(coroutine, current_timer_coroutine_id_) &&
    scheduler->CoroutineStatus(coroutine, current_task_coroutine_id_)*/){
    scheduler->CoroutineResume(coroutine, current_coroutine_id_);
    //scheduler->CoroutineResume(coroutine, current_timer_coroutine_id_);
    //scheduler->CoroutineResume(coroutine, current_task_coroutine_id_);
  }

  while (data->is_run){
    if (data->data_.empty()) {
      usleep(1);
      continue;
    }

    std::queue<EventMessage*> temp;
    data->lock_.lock();
    temp.swap(data->data_);
    data->lock_.unlock();
    ProcessMessage(temp);
  }
  TRACE(logger_, "CoroutineServiceHandler::Run will be exit.");
}

void CoroutineServiceHandler::ProcessCoroutine(CoroutineActor* coroutine, void* data){
  TRACE(logger_, "CoroutineServiceHandler::Process. thread id:"<< ThreadId::Get());
  HandleData* handle_data = (HandleData*)data;
  CoroutineServiceHandler* handle = 
    dynamic_cast<CoroutineServiceHandler*>(handle_data->handle_);
  if (handle == nullptr){
    ERROR(logger_, "handle is null prt...");
    return;
  }

  while (handle_data->is_run) {
    handle->ProcessTimer();
    handle->ProcessTask(handle_data);
    handle->Process(handle_data);
  }
  TRACE(logger_, "CoroutineServiceHandler thread will be exit.");
}

void CoroutineServiceHandler::Resume(int coroutine_id){
  if (coroutine_id < 0){
    TRACE(logger_, "Coroutine not created...");
    return;
  }
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  CoroutineActor* coroutine = CoroutineContext::Instance().GetCoroutine();
  if (scheduler->CoroutineStatus(coroutine, coroutine_id)) {
    TRACE(logger_, "CoroutineResume coroutine:" << coroutine << ",id:" << coroutine_id);
    scheduler->CoroutineResume(coroutine, coroutine_id);
  }
}

void CoroutineServiceHandler::ProcessTimer() {
  timer_.ProcessTimer();

  //if (0 == process_times){
  //  Resume(current_timer_coroutine_id_);
  //}
}

//when send receive timeout
void CoroutineServiceHandler::OnTimer(void* function_data){
  TRACE(logger_, "CoroutineServiceHandler::OnTimer.");
  Resume(current_coroutine_id_);
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

  //Resume(current_task_coroutine_id_);
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

void CoroutineServiceHandler::ProcessItem(EventMessage* msg){
  Connecting* con = (Connecting*)((void*)(msg->descriptor_id));
  if (con->GetIsServer()) {
    //处理服务端接收的消息
  } else {
    //处理客户端接收的消息
    CoroutineActor* coroutine = CoroutineContext::Instance().GetCoroutine();
    coroutine->SendMessage(msg);

    Resume(current_coroutine_id_);
  }
}

void CoroutineServiceHandler::ProcessMessage(std::queue<EventMessage*>& msg_list){
  TRACE(logger_, "handle id:"<<GetHandlerId()
    <<",ProcessMessage size:"<< msg_list.size());
  while (!msg_list.empty()) {
    EventMessage *msg = msg_list.front();
    msg_list.pop();
    
    ProcessItem(msg);
  }
}

void CoroutineServiceHandler::ProcessMessageHandle(std::queue<EventMessage*>& msg_list) {
  TRACE(logger_, "handle id:" << GetHandlerId()
    << ",ProcessMessage size:" << msg_list.size());
  while (!msg_list.empty()) {
    EventMessage *msg = msg_list.front();
    msg_list.pop();
    ProcessItem(msg);
    Handle(msg);
  }
}

}

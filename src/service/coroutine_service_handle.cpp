
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
#include "service/service_manager.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineServiceHandler);

void CoroutineServiceHandler::Run(HandleData* data){
  prctl(PR_SET_NAME, "CoroutineServiceHandler");
  INFO(logger_, "CoroutineServiceHandler::Run,thread id:"<< ThreadId::Get());
  CoroutineContext::Instance().Init(this,&time_mgr_);
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  int coroutine_size = service::GetServiceManager().GetServiceConfig().coroutine_size;
  scheduler->InitCoroSchedule(
    &CoroutineServiceHandler::ProcessCoroutine, data, coroutine_size);
  int coro_id = scheduler->GetAvailableCoroId();
  scheduler->CoroutineResume(coro_id);

  while (data->is_run) {
    //这里负责切换到协程，所有业务都在协程中处理
    if (scheduler->CoroutineResumeActive()){
      continue;
    }

    if (CoroutineContext::GetCurCoroutineId() < 0) {
      int coro_id = scheduler->GetAvailableCoroId();
      TRACE(logger_, "CoroutineResume available coro id:"<< coro_id);
      if (coro_id < 0){
        continue;
      }
      scheduler->CoroutineResume(coro_id);
    }else{
      WARN(logger_, "not to here,coro id:"<< CoroutineContext::GetCurCoroutineId());
    }
  }
  INFO(logger_, "CoroutineServiceHandler::Run exit.");
}

static void ProcessDebug(){
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  scheduler->ProcessDebug();
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
    handle->ProcessTimer();
    handle->ProcessTask(handle_data);
    handle->Process(handle_data);
    ProcessDebug();
  }
  TRACE(logger_, "CoroutineServiceHandler thread will be exit.");
}

void CoroutineServiceHandler::ProcessTimer() {
  time_mgr_.RunTimer();
}

//when send receive timeout
void CoroutineServiceHandler::OnTimerCoro(void* function_data, int& coro_id){
  TRACE(logger_, "CoroutineServiceHandler::OnTimer.");
  int coro_id_tmp = *(int*)(function_data);
  if (coro_id_tmp < 0) {
    //not to here,否则会丢消息
    ERROR(logger_, "error coro_id:" << coro_id);
    return;
  }

  if (coro_id_tmp != coro_id){
    WARN(logger_, "may be error tmp coro_id:" << coro_id_tmp << ",coro_id" << coro_id);
  }

  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  //本来就在协程里面，会先切出来，然后切到另外一个协程
  if (!scheduler->CoroutineYieldToActive(coro_id_tmp)) {
    TRACE(logger_, "yield failed, coro_id:" << coro_id_tmp);
  }
}

void CoroutineServiceHandler::ProcessTask(HandleData* data){
  if (data->task_.empty()){
    return;
  }

  TRACE(logger_, "CoroutineServiceHandler::ProcessTask.");
  std::queue<Task*> temp;
  data->lock_task_.lock();
  temp.swap(data->task_);
  data->lock_task_.unlock();

  while (!temp.empty()) {
    Task *task = temp.front();
    temp.pop();
    task->OnTask(nullptr);
  }

}

void CoroutineServiceHandler::Process(HandleData* data){
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  int static empty_times = 0;
  if (data->data_.empty()) {
    usleep(100);
    empty_times++;
    if (0 == empty_times % 10){
      scheduler->CoroutineYield();
    }
    return;
  }
  empty_times = 0;
  std::queue<EventMessage*> temp;
  data->lock_.lock();
  temp.swap(data->data_);
  data->lock_.unlock();

  ProcessMessageHandle(temp);
}

void CoroutineServiceHandler::ProcessClientItem(EventMessage* msg){
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  if (msg->coroutine_id <0){
    //not to here,否则会丢消息
    ERROR(logger_, "error coro_id:" << msg->coroutine_id);
    scheduler->CoroutineYield();
    MessageFactory::Destroy(msg);
    return;
  }
  CoroutineActor* coroutine = scheduler->GetCoroutineCtx(msg->coroutine_id);
  if (coroutine->SendMessage(msg)){
    //本来就在协程里面，会先切出来，然后切到另外一个协程
    if (!scheduler->CoroutineYieldToActive(msg->coroutine_id)) {
      WARN(logger_, "client yield failed, coro_id:" << msg->coroutine_id);
    }
  }else{
    //超时的
    MessageFactory::Destroy(msg);
  }
}

void CoroutineServiceHandler::ProcessMessageHandle(std::queue<EventMessage*>& msg_list) {
  TRACE(logger_, "handle id:" << GetHandlerId()
    << ",ProcessMessage size:" << msg_list.size());
  CoroutineSchedule* scheduler = CoroutineContext::Instance().GetScheduler();
  while (!msg_list.empty()) {
    EventMessage *msg = msg_list.front();
    msg_list.pop();

    TRACE(logger_, "message is:" << *msg);
    if (MessageBase::kStatusOK != msg->status){
      //超时或者连接被关闭等无效消息
      scheduler->CoroutineYield();
      MessageFactory::Destroy(msg);
      continue;
    }
    if (msg->direction == MessageBase::kIncomingRequest){
      //处理服务端接收的消息
      uint64_t birth_to_now = Time::GetCurrentClockTime()- msg->birthtime;
      if (birth_to_now > INNER_QUERY_SEND_PROTECT_TIME){
        //从io handle到service handle超过100ms,服务端过载保护
        INFO(logger_, "birth_to_now:"<< birth_to_now <<",msg:" << *msg);
        MessageFactory::Destroy(msg);
        continue;
      }
      Handle(msg);
    }else{
      //处理客户端接收的消息
      ProcessClientItem(msg);
    }
  }
}

void CoroTimer::OnTimer(void* timer_data, uint64_t time_id){
  int coro_id_tmp = *(int*)(timer_data);
  service_handle_->OnTimerCoro(timer_data, coro_id_tmp);
  //delete this;
}

}

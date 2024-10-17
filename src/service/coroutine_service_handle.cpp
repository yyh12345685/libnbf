
#include <functional>
#include <unistd.h>
#include <sys/prctl.h>
#include "common/thread_id.h"
#include "service/coroutine_service_handle.h"
#include "coroutine/coroutine_manager.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_actor.h"
#include "coroutine/coro_context.h"
#include "handle_data.h"
#include "task.h"
#include "net/connect.h"
#include "service/service_manager.h"
#include "common/time.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineServiceHandler);

void CoroutineServiceHandler::Run(HandleData* data) {
  prctl(PR_SET_NAME, "CoroutineServiceHandler");
  INFO(logger_, "CoroutineServiceHandler::Run,thread id:"<< ThreadId::Get());
  CoroutineManager::Instance().Init(this, &time_mgr_);
  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  int coroutine_size = service::GetServiceManager().GetServiceConfig().coroutine_size;
  uint32_t coroutine_type = service::GetServiceManager().GetServiceConfig().coroutine_type;
  int stack_size = service::GetServiceManager().GetServiceConfig().coroutine_stack_size;
  scheduler->InitCoroSchedule(
    &CoroutineServiceHandler::ProcessCoroutine, data, coroutine_size, coroutine_type, stack_size);
  CoroContext* coro = scheduler->GetAvailableCoro();
  if (coro == nullptr) {
    WARN(logger_, "CoroutineServiceHandler::Run exit becase coro is nullptr.");
    return;
  }
  scheduler->ScCoroutineResume(coro);
  debug_time_ = time(NULL);

  while (data->is_run) {
    // 优先切到已经有数据的协程
    if (scheduler->CoroutineResumeActive()) {
      continue;
    }
    CoroContext* loop_coro = scheduler->GetAvailableCoro();
    if (loop_coro == nullptr) {
      WARN(logger_, "CoroutineServiceHandler::Run no coro maybe is limited.");
      continue;
    }
    // TRACE(logger_, "CoroutineServiceHandler resume loop coro:" << loop_coro);
    // 第一个coro放这里做循环
    scheduler->ScCoroutineResume(loop_coro);
  }
  INFO(logger_, "CoroutineServiceHandler::Run exit.");
}

static void ProcessDebug(){
  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  scheduler->ProcessDebug();
}

void CoroutineServiceHandler::ProcessCoroutine(void* data){
  TRACE(logger_, "CoroutineServiceHandler::ProcessCoroutine. thread id:"<< ThreadId::Get());
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
  // 延迟的定时任务
  time_mgr_.RunTimer();

  // 客户端超时处理
  /*std::queue<CoroContext*> temp;
  {
    std::unique_lock<std::mutex> lock(time_out_lock_);
    if (time_out_coro_.empty()) {
      return;
    }  
    temp.swap(time_out_coro_);
  } 
  while (!temp.empty()) {
    CoroContext *msg_coro = temp.front();
    temp.pop();
    CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
    // 本来就在协程里面，会先切出来，然后切到另外一个协程，这个可以直接切吗，还是要先挂起当前协程？
    if (!scheduler->CoroutineYieldToActive(msg_coro)) {
      WARN(logger_, "add coror to run failed, msg_coro:" << msg_coro);
    }
  }*/
}

//when send receive timeout
void CoroutineServiceHandler::OnTimerCoro(void* function_data) {
  TRACE(logger_, "CoroutineServiceHandler::OnTimer.");
  CoroContext* msg_coro = (CoroContext*)(function_data);
  if (nullptr == msg_coro) {
    //not to here,否则会丢消息
    ERROR(logger_, "error coro data:" << function_data);
    return;
  }

  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  // 本来就在协程里面，会先切出来，然后切到另外一个协程，这个可以直接切吗，还是要先挂起当前协程？
  if (!scheduler->CoroutineYieldToActive(msg_coro)) {
    WARN(logger_, "add coror to run failed, msg_coro:" << msg_coro);
  }
}

/*void CoroutineServiceHandler::AddTimeOutFromClient(CoroContext* msg_coro) {
  time_out_lock_.lock();
  time_out_coro_.push(msg_coro);
  time_out_lock_.unlock();
}*/

void CoroutineServiceHandler::ProcessTask(HandleData* data) {
  TRACE(logger_, "CoroutineServiceHandler::ProcessTask.");
  std::queue<Task*> temp;
  {
    std::unique_lock<std::mutex> lock(data->task_lock_);
    if (data->task_.empty()) {
      return;
    }
    temp.swap(data->task_);
  }
  
  /*if (data->PopAllTask(temp, 1)) {
    return;
  }*/

  while (!temp.empty()) {
    Task *task = temp.front();
    temp.pop();
    task->OnTask(nullptr);
  }
}

void CoroutineServiceHandler::Process(HandleData* data) {
  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  std::queue<EventMessage*> temp;
  {
    std::unique_lock<std::mutex> lock(data->data_lock_);
    temp.swap(data->data_);
  }

  if (temp.empty()) {
    usleep(100);
    scheduler->CoroutineYield();
    return;
  }

  /*if (data->PopAllData(temp, 1)) {
    return;
  }*/

  ProcessMessageHandle(temp);
}

void CoroutineServiceHandler::ProcessClientItem(EventMessage* msg) {
  CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  if (nullptr == msg->msg_coro){
    //not to here,否则会丢消息
    ERROR(logger_, "msg_coro is null msg:" << msg);
    scheduler->CoroutineYield();
    MessageFactory::Destroy(msg);
    return;
  }

  CoroutineActor* coroutine = msg->msg_coro->actor;
  CoroContext* msg_coro = msg->msg_coro;
  if (!coroutine->SendMessage(msg)) {
    //超时的
    MessageFactory::Destroy(msg);
  }
  //本来就在协程里面，会先切出来，然后切到另外一个协程
  if (!scheduler->CoroutineYieldToActive(msg_coro)) {
    WARN(logger_, "client yield failed, coro:" << msg_coro);
  }
}

void CoroutineServiceHandler::ProcessMessageHandle(std::queue<EventMessage*>& msg_list) {
  TRACE(logger_, "handle id:" << GetHandlerId()
    << ",ProcessMessage size:" << msg_list.size());
  //CoroutineSchedule* scheduler = CoroutineManager::Instance().GetScheduler();
  size_t handle_size = msg_list.size();
  while (!msg_list.empty()) {
    //for debug begin-------------
    time_t cur_time = time(NULL);
    if ((cur_time - debug_time_) > 60 && handle_size == msg_list.size()) {
      INFO(logger_, "ThreadId:" << ThreadId::Get()
        << ",handle size:" << handle_size);
      debug_time_ = cur_time;
    }
    //for debug end---------------
    EventMessage *msg = msg_list.front();
    msg_list.pop();

    TRACE(logger_, "ProcessMessageHandle message is:" << *msg);
    /*if (MessageBase::kStatusOK != msg->status) {
      //超时或者连接被关闭等无效消息
      scheduler->CoroutineYield();
      MessageFactory::Destroy(msg);
      continue;
    }*/
    if (msg->direction == MessageBase::kIncomingRequest) {
      //处理服务端接收的消息
      uint64_t birth_to_now = Time::GetCurrentClockTime()- msg->birthtime;
      if (birth_to_now > INNER_QUERY_SEND_PROTECT_TIME) {
        //从io handle到service handle超过100ms,服务端过载保护
        INFO(logger_, "ProcessMessageHandle birth_to_now_ms:" << birth_to_now << ",msg:" << *msg);
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
  //service_handle_->OnTimerCoro(timer_data);
  //delete this;
}

}

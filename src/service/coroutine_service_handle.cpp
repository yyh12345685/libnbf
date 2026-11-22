
#include <functional>
#include <unistd.h>
#include <sys/prctl.h>
#include "common/thread_id.h"
#include "service/coroutine_service_handle.h"
#include "coroutine/coroutine_manager.h"
#include "coroutine/coro_context.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_actor.h"
#include "handle_data.h"
#include "task.h"
#include "net/connect.h"
#include "service/service_manager.h"
#include "common/time.h"
#include "monitor/matrix_scope.h"
#include "context.h"

namespace bdf {

LOGGER_CLASS_IMPL(logger_, CoroutineServiceHandler);

void CoroutineServiceHandler::Run(HandleData* data){
  prctl(PR_SET_NAME, "CoroutineServiceHandler");
  INFO(logger_, "CoroutineServiceHandler::Run,thread id:"<< ThreadId::Get());
  CoroutineManager::Init(this, &time_mgr_);
  CoroutineSchedule* scheduler = CoroutineManager::GetScheduler();
  const auto& config = service::GetServiceManager().GetServiceConfig();
  max_coroutine_size_ = config.max_coroutine_size;
  scheduler->InitCoroSchedule(
    &CoroutineServiceHandler::ProcessCoroutine, data, config.coroutine_size,
    config.coroutine_type, config.coroutine_stack_size);
  debug_time_ = time(NULL);

  while (data->is_run) {
    ProcessTimer();
    if (scheduler->CoroutineResumeActive()) {
      continue;
    }

    CoroContext* coro = scheduler->GetAvailableCoro();
    if (!coro) {
      usleep(100);
      continue;
    }
    scheduler->ScCoroutineResume(coro);
  }
  INFO(logger_, "CoroutineServiceHandler::Run exit.");
}

static void ProcessDebug(){
  CoroutineSchedule* scheduler = CoroutineManager::GetScheduler();
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
void CoroutineServiceHandler::OnTimerCoro(void* function_data) {
  auto* scheduler = CoroutineManager::GetScheduler();
  auto* coro = static_cast<CoroContext*>(function_data);
  scheduler->CoroutineYieldToActive(coro);
}

void CoroutineServiceHandler::ProcessTask(HandleData* data){
  std::queue<Task*> temp;
  {
    // task_ is pushed by other threads under lock_task_: read it under it too.
    std::lock_guard<std::mutex> guard(data->task_lock_);
    if (data->task_.empty()) {
      return;
    }
    temp.swap(data->task_);
  }
  TRACE(logger_, "CoroutineServiceHandler::ProcessTask.");

  while (!temp.empty()) {
    Task *task = temp.front();
    temp.pop();
    task->OnTask(nullptr);
  }

}

void CoroutineServiceHandler::Process(HandleData* data){
  CoroutineSchedule* scheduler = CoroutineManager::GetScheduler();
  // One counter per handler thread (a function static was shared by all).
  static thread_local int empty_times = 0;
  if (pending_.empty()) {
    std::lock_guard<std::mutex> guard(data->data_lock_);
    pending_.swap(data->data_);
  }
  if (pending_.empty()) {
    usleep(200);
    empty_times++;
    if (0 == empty_times % 10){
      scheduler->CoroutineYield();
    }
    return;
  }
  empty_times = 0;
  ProcessMessageHandle(pending_);
  scheduler->CoroutineYield();
}

void CoroutineServiceHandler::ProcessClientItem(EventMessage* msg) {
  auto* scheduler = CoroutineManager::GetScheduler();
  CoroContext* coro = msg->msg_coro;
  if (!scheduler->IsWaiting(coro) || !coro->actor || !coro->actor->SendMessage(msg)) {
    MessageFactory::Destroy(msg);
    return;
  }
  scheduler->CoroutineYieldToActive(coro);
}

void CoroutineServiceHandler::ProcessMessageHandle(std::queue<EventMessage*>& msg_list) {
  TRACE(logger_, "handle id:" << GetHandlerId()
    << ",ProcessMessage size:" << msg_list.size());
  CoroutineSchedule* scheduler = CoroutineManager::GetScheduler();
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

    TRACE(logger_, "message is:" << *msg);
    if (MessageBase::kStatusOK != msg->status
      && msg->direction == MessageBase::kIncomingRequest){
      //超时或者连接被关闭等无效消息
      service::GetServiceManager().DropRequest(msg);
      continue;
    }
    // A failed client response (connection closed, timeout) still goes to
    // its waiting coroutine, which sees the status and returns at once
    // instead of sleeping until its own timeout.
    if (msg->direction == MessageBase::kIncomingRequest){
      //处理服务端接收的消息
      uint64_t birth_to_now = Time::GetCurrentClockTime()- msg->birthtime;
      if (birth_to_now > INNER_QUERY_SEND_PROTECT_TIME){
        //从io handle到service handle超过100ms,服务端过载保护
        INFO(logger_, "birth_to_now:"<< birth_to_now <<",msg:" << *msg);
        service::GetServiceManager().DropRequest(msg);
        continue;
      }
      // Coroutines are created on demand, so a slow downstream would grow
      // them (and their stacks) without bound while the queue still drains
      // fast enough that the age check above never fires. Shed instead.
      if (scheduler->SuspendedCount() >= max_coroutine_size_) {
        monitor::MatrixCounter::Add("server_coroutine_overload", 1);
        static thread_local time_t last_log = 0;
        if (time(NULL) != last_log) {
          last_log = time(NULL);
          WARN(logger_, "coroutine limit reached, shed request,suspended:"
            << scheduler->SuspendedCount());
        }
        service::GetServiceManager().DropRequest(msg);
        continue;
      }
      Handle(msg);
    }else if (nullptr != msg->ctx && nullptr != msg->ctx->callback){
      // Invoke() response: run the callback (and MarkEnd) like the plain
      // handler does. It used to go to ProcessClientItem, which found no
      // waiting coroutine and destroyed it, so the callback never ran.
      Handle(msg);
    }else{
      //处理客户端接收的消息
      ProcessClientItem(msg);
    }
  }
}

void CoroTimer::OnTimer(void* timer_data, uint64_t time_id){
  service_handle_->OnTimerCoro(timer_data);
}

}

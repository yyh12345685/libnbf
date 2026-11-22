
#include "coroutine_actor_c.h"
#include "coroutine/coroutine_schedule.h"
#include "coroutine/coroutine_manager.h"
#include "service/coroutine_service_handle.h"
#include "message.h"
#include "common/thread_id.h"

namespace bdf{

LOGGER_CLASS_IMPL(logger_, CoroutineActorc);

CoroutineActorc::CoroutineActorc() :
  is_waiting_(false),
  waiting_id_(-1) {
  msg_list_.clear();
}

CoroutineActorc::~CoroutineActorc() {
  for (auto* message : msg_list_) {
    MessageFactory::Destroy(message);
  }
}

EventMessage* CoroutineActorc::RecieveMessage(uint64_t sequence_id, uint32_t timeout_ms) {
  auto* scheduler = CoroutineManager::GetScheduler();
  auto* timer_mgr = CoroutineManager::GetTimerMgr();
  auto* handler = CoroutineManager::GetServiceHandler();
  auto* coro = CoroutineManager::GetCurrentCoroutine();
  if (!coro || coro->actor != this || !timer_mgr || !handler) {
    is_waiting_ = false;
    return nullptr;
  }

  if (msg_list_.empty()) {
    CoroTimer timer(handler, coro);
    const auto timer_id = timer_mgr->AddTimer(&timer, timeout_ms);
    scheduler->ReceiveCoroutineYield();
    timer_mgr->DelTimer(timer_id);
  }

  is_waiting_ = false;
  waiting_id_ = -1;
  if (msg_list_.empty()) return nullptr;
  EventMessage* response = msg_list_.front();
  msg_list_.pop_front();
  if (response->sequence_id != sequence_id) {
    MessageFactory::Destroy(response);
    return nullptr;
  }
  return response;
}

bool CoroutineActorc::SendMessage(EventMessage* message) {
  TRACE(logger_, "SendMessage,is_waiting_" << is_waiting_
    << ",sequence id:" << message->sequence_id << ",waiting_id:" << waiting_id_
    << ",msg_list_size:" << msg_list_.size());
  if (!is_waiting_ || message->sequence_id != waiting_id_ || !msg_list_.empty()) {
    return false;
  }
  TRACE(logger_, "ThreadId:" << ThreadId::Get() << ",response message:" << *message);
  msg_list_.emplace_back(message);
  return true;
}

}

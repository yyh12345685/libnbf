
#include "message_base.h"

namespace bdf {

std::string MessageBase::ToString() const {
  std::ostringstream oss;
  Dump(oss);
  return oss.str();
}

void MessageBase::Dump(std::ostream& os) const {
  os << "{\"type\": \"MessageBase\""
    << ", \"type_id\": " << MessageType::ToString(type_id)
    << ", \"type_io_event\": " << MessageType::ToIoEventString(type_io_event)
    << ", \"direction\": " << ToDirectionString(direction)
    << ", \"status\": " << ToStatusString(status)
    << ", \"sequence_id\": " << sequence_id
    << ", \"birthtime\": " << birthtime
    << ", \"event_mask\": " << event_mask
    << "}";
};

EventMessage::~EventMessage() {
  if (ctx) {
    if (ctx->monitor_id && monitor::GlobalMatrix::Ready()) {
      monitor::GlobalMatrix::Instance().MarkEnd(ctx->monitor_id, false);
    }
    BDF_DELETE(ctx);
  }
}

void EventMessage::Dump(std::ostream& os) const {
  os << "{\"type\": \"EventMessage\""
    << ", \"descriptor_id\": " << descriptor_id
    << ", \"handle_id\": " << handle_id
    << ", \"timer_out_id\": " << timer_out_id
    << ", \"coro ptr:\": " << msg_coro
    << ", \"messagebase\": " << ToString();
  os << "}";
}

std::ostream& operator << (std::ostream& os, const EventMessage& message) {
  message.Dump(os);
  return os;
}

}

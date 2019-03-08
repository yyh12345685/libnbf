
#include "message_base.h"

namespace bdf {

std::string MessageBase::ToString() const {
  std::ostringstream oss;
  Dump(oss);
  return oss.str();
}

void MessageBase::Dump(std::ostream& os) const {
  os << "{\"type\": \"MessageBase\""
     << ", \"type_id\": " << type_id << "}";
};


void EventMessage::Dump(std::ostream& os) const {
  os << "{\"type\": \"EventMessage\""
    << ", \"descriptor_id\": " << descriptor_id
    << ", \"messagebase\": " << ToString();
  os << "}";
}

std::ostream& operator << (std::ostream& os, const EventMessage& message) {
  message.Dump(os);
  return os;
}

}

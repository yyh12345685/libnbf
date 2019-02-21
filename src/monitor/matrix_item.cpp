
#include "matrix_item.h"

namespace bdf {

namespace monitor {

void MatrixItem::Dump(std::ostream& os) const {
  os << "{\"operation\": " << (int)operation
     << ",\"persistenc\": " << persistent
     << ",\"val\": " << val
     << ",\"name\": \"" << name << "\""
     << ",\"result\": \"" << result << "\""
     << "}";
}

std::ostream& operator << (std::ostream& os, const MatrixItem& item) {
  item.Dump(os);
  return os;
}

}
}


#pragma once

#include <stdint.h>
#include <iostream>
#include <string>

namespace bdf {

namespace monitor {

class MatrixItem {
 public:
  MatrixItem() = default;

  MatrixItem(int8_t _operation, const std::string& _name, int64_t _val, bool _persistent = false)
      : persistent(_persistent)
      , operation(_operation)
      , val(_val)
      , name(_name) {
  }

  bool persistent;
  int8_t operation;
  int64_t val;
  std::string name;
  std::string result;

  void Dump(std::ostream& os) const;
};

std::ostream& operator << (std::ostream& os, const MatrixItem& item);

}
}

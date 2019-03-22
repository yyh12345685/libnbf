#pragma once

#include <stdint.h>

namespace bdf {

class Task {
public:
  virtual void OnTask(void* function_data) = 0;
  virtual ~Task() { }
};

}

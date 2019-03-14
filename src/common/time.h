
#pragma once

#include <time.h>
#include <sys/time.h>
#include <memory.h>
#include <string>
#include "string_util.h"

namespace bdf {

class Time {
 public:

  inline static uint64_t GetMillisecond() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return  (tv.tv_usec/1000 + tv.tv_sec*1000);
  }

  inline static uint64_t GetMicrosecond() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_usec + tv.tv_sec * 1000000);
  }

};

}

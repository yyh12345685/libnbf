
#pragma once

#include <time.h>
#include <sys/time.h>
#include <memory.h>
#include <string>
#include "string_util.h"

namespace bdf {

#define REGET_TIME_US_GTOD   1
#define CPU_SPEED_GB    1
#define TIMES 1000

#ifdef __x86_64__
#define RDTSC() \
  ({ register uint32_t a,d; __asm__ __volatile__( "rdtsc" : "=a"(a), "=d"(d)); (((uint64_t)a)+(((uint64_t)d)<<32)); })
#else
#define RDTSC() \
  ({ register uint64_t tim; __asm__ __volatile__( "rdtsc" : "=A"(tim)); tim; })
#endif

#ifdef __x86_64__
#define ASUFFIX "q"
#else
#define ASUFFIX "l"
#endif

#define CAS(ptr, val_old, val_new) \
  ({ char ret; __asm__ __volatile__("lock; cmpxchg" ASUFFIX " %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})

class Time {
 public:

   inline static int GetTimeOfDay(struct timeval *tv, void *not_used){
     static volatile uint64_t walltick;
     static volatile struct timeval walltime;
     static volatile long lock = 0;
     const unsigned int max_ticks = CPU_SPEED_GB * TIMES * REGET_TIME_US_GTOD;

     if (walltime.tv_sec == 0 || (RDTSC() - walltick) > max_ticks){
       // try lock
       if (lock == 0 && CAS(&lock, 0UL, 1UL)) {
         gettimeofday((struct timeval*)&walltime, NULL);
         walltick = RDTSC();
         lock = 0; // unlock
       } else {
         // try lock failed, use system time
         //int gettimeofday(struct timeval *tv, struct timezone *tz);
         return gettimeofday(tv, (struct timezone *)not_used);
       }
     }
     memcpy(tv, (void*)&walltime, sizeof(struct timeval));
     return 0;
   }

  //测试下来，有优化的版本貌似计算不是特别准确
  //毫秒
  inline static uint64_t GetMillisecond() {
    struct timeval tv;
    GetTimeOfDay(&tv, NULL);
    return  (tv.tv_usec/1000 + tv.tv_sec*1000);
  }

  //测试下来，有优化的版本貌似计算不是特别准确,优先使用clock_gettime吧
  //微秒
  inline static uint64_t GetMicrosecond() {
    struct timeval tv;
    GetTimeOfDay(&tv, NULL);
    return (tv.tv_usec + tv.tv_sec * 1000000);
  }

  //毫秒
  inline static uint64_t GetMillisecondOri() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return  (tv.tv_usec / 1000 + tv.tv_sec * 1000);
  }

  //无优化的版本占用系统cpu较高，优先使用clock_gettime吧
  //微秒
  inline static uint64_t GetMicrosecondOri() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_usec + tv.tv_sec * 1000000);
  }

  //系统启动那一刻到当前时间的毫秒数
  //毫秒
  inline static uint64_t GetCurrentClockTime(){
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t now = (uint64_t)tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
    return now;
  }

  //系统启动那一刻到当前时间的微秒数
  //微秒
  inline static uint64_t GetCurrentClockTimeus() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t now = (uint64_t)tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
    return now;
  }

};

}

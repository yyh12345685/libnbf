#pragma once

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef likely
#define likely(x) __builtin_expect ((x), true)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect ((x), false)
#endif

//内部队列扭转过载保护时长 100ms
#define INNER_QUERY_SEND_PROTECT_TIME 100

#define DISALLOW_COPY_AND_ASSIGN(class_name)              \
private:                                                  \
  class_name(const class_name&);                          \
  void operator=(const class_name&);

#define DISALLOW_COPY_ASSIGN_CONSTRUCTION(class_name)     \
private:                                                  \
  class_name() {}                                         \
  virtual ~class_name() {}                                \
  DISALLOW_COPY_AND_ASSIGN(class_name)                    \


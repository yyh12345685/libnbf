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

#define DISALLOW_COPY_AND_ASSIGN(class_name)              \
private:                                                  \
  class_name(const class_name&);                          \
  void operator=(const class_name&);

#define DISALLOW_COPY_ASSIGN_CONSTRUCTION(class_name)     \
private:                                                  \
  class_name() {}                                         \
  virtual ~class_name() {}                                \
  DISALLOW_COPY_AND_ASSIGN(class_name)                    \


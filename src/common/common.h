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

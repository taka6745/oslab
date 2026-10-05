#include "types.h"
// Caller validates HPET period in 1..1e8 femtoseconds. Exact conversion avoids
// long-uptime multiplication overflow and a freestanding 128-bit division
// helper.
uint64_t clock_milliseconds(uint64_t count, uint64_t period) {
  uint64_t low, high, scale = 1000000000000ull;
  __asm__ volatile("mulq %3; divq %4"
                   : "=&a"(low), "=&d"(high)
                   : "0"(count), "r"(period), "r"(scale)
                   : "cc");
  return low;
}

#ifndef OSLAB_CPU_H
#define OSLAB_CPU_H
#include "os.h"
#if defined(__aarch64__)
static inline uint64_t cycles(void) {
  uint64_t value;
  __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(value) :: "memory");
  return value;
}
#elif defined(__x86_64__)
#include "x86.h"
#else
#error unsupported CPU
#endif
#endif

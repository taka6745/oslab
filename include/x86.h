#ifndef OSLAB_X86_H
#define OSLAB_X86_H
#include "os.h"
static inline void out8(uint16_t p, uint8_t v) {
  __asm__ volatile("outb %0,%1" ::"a"(v), "Nd"(p));
}
static inline uint8_t in8(uint16_t p) {
  uint8_t v;
  __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p));
  return v;
}
static inline void out32(uint16_t p, uint32_t v) {
  __asm__ volatile("outl %0,%1" ::"a"(v), "Nd"(p));
}
static inline uint32_t in32(uint16_t p) {
  uint32_t v;
  __asm__ volatile("inl %1,%0" : "=a"(v) : "Nd"(p));
  return v;
}
static inline void barrier(void) { __asm__ volatile("" ::: "memory"); }
static inline uint64_t cycles(void) {
  uint32_t lo, hi;
  __asm__ volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi)::"memory");
  return (uint64_t)hi << 32 | lo;
}
#endif

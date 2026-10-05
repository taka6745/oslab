#include "os.h"
#if !OSLAB_MACHINE
void *memcpy(void *d, const void *s, size_t n) {
  void *result = d;
  size_t words = n / 8;
  __asm__ volatile("rep movsq" : "+D"(d), "+S"(s), "+c"(words)::"memory");
  n %= 8;
  __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n)::"memory");
  return result;
}
void *memset(void *d, int c, size_t n) {
  uint8_t *a = d;
  for (size_t i = 0; i < n; i++)
    a[i] = (uint8_t)c;
  return d;
}
#endif
int memcmp(const void *a, const void *b, size_t n) {
  const uint8_t *x = a, *y = b;
  for (size_t i = 0; i < n; i++)
    if (x[i] != y[i])
      return x[i] < y[i] ? -1 : 1;
  return 0;
}
size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n])
    n++;
  return n;
}

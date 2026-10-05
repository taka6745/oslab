#include "os.h"
void number(uint64_t n, unsigned base) {
  char b[24];
  size_t i = 0;
  if (base != 10 && base != 16)
    return;
  do {
    b[i++] = "0123456789abcdef"[n % base];
    n /= base;
  } while (n);
  while (i)
    putc_os(b[--i]);
}
void field(const char *s, uint64_t n) {
  puts_os(" ");
  puts_os(s);
  putc_os('=');
  number(n, 10);
}
void ip_print(uint32_t ip) {
  for (int i = 3; i >= 0; i--) {
    number((ip >> (i * 8)) & 255, 10);
    if (i)
      putc_os('.');
  }
}

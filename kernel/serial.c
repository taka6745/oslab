#include "os.h"
#include "x86.h"
void serial_init(void) {
  out8(0x3fb, 0x80);
  out8(0x3f8, 1);
  out8(0x3f9, 0);
  out8(0x3fb, 3);
  out8(0x3fa, 0xc7);
  out8(0x3fc, 0x0b);
}
void putc_os(char c) {
  out8(0xe9, (uint8_t)c);
  for (unsigned i = 0; i < 100000; i++)
    if (in8(0x3fd) & 0x20) {
      out8(0x3f8, (uint8_t)c);
      return;
    }
}
void puts_os(const char *s) {
  while (*s)
    putc_os(*s++);
}
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
int serial_get(void) { return in8(0x3fd) & 1 ? in8(0x3f8) : -1; }
_Noreturn void exit_os(bool ok) {
  out32(0xf4, ok ? 0x10 : 0x11);
  __asm__ volatile("cli");
  for (;;)
    __asm__ volatile("hlt");
}
_Noreturn void panic(const char *s) {
  __asm__ volatile("cli");
  puts_os("OSL1 PANIC reason=");
  puts_os(s);
  putc_os('\n');
  for (;;)
    __asm__ volatile("hlt");
}

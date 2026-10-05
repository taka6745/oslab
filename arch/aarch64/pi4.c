#include "http.h"
// BCM2711 ARM Peripherals: GPIO and PL011, low peripheral map.
// Bring-up image only. No GENET Ethernet implementation is advertised here.
static volatile uint32_t *const uart = (void *)0xfe201000;
static volatile uint32_t *const gpio = (void *)0xfe200000;
void putc_os(char c) {
  while (uart[0x18 / 4] & (1u << 5))
    __asm__ volatile("yield");
  uart[0] = (uint8_t)c;
}
void puts_os(const char *s) {
  while (*s)
    putc_os(*s++);
}
static void serial(void) {
  uart[0x30 / 4] = 0;
  gpio[1] = (gpio[1] & ~((7u << 12) | (7u << 15))) | (4u << 12) | (4u << 15);
  gpio[0xe4 / 4] &= ~(15u << 28);
  uart[0x44 / 4] = 0x7ff;
  // Firmware config must set init_uart_clock=48000000.
  uart[0x24 / 4] = 26;
  uart[0x28 / 4] = 3;
  uart[0x2c / 4] = (3u << 5) | (1u << 4);
  uart[0x38 / 4] = 0;
  uart[0x30 / 4] = (1u << 9) | (1u << 8) | 1;
}
static uint64_t cache_size(uint64_t selector) {
  uint64_t id;
  __asm__ volatile("msr csselr_el1, %1; isb; mrs %0, ccsidr_el1"
                   : "=r"(id)
                   : "r"(selector));
  return (((id >> 13) & 32767) + 1) * (((id >> 3) & 1023) + 1) *
         (1ull << ((id & 7) + 4));
}
void pi_main(void) {
  serial();
  puts_os("OSL1 BOOT pi4 aarch64 el1\n");
  uint64_t control;
  __asm__ volatile("mrs %0, sctlr_el1" : "=r"(control));
  puts_os("OSL1 CACHE");
  field("sctlr_el1", control);
  field("l1i_bytes", cache_size(1));
  field("l1d_bytes", cache_size(0));
  field("l2_bytes", cache_size(2));
  putc_os('\n');
  uint64_t frequency, initial, now;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
  __asm__ volatile("mrs %0, cntpct_el0" : "=r"(initial));
  if (!frequency) {
    puts_os("OSL1 ERROR counter-frequency-zero\n");
    return;
  }
  do {
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
  } while (now - initial < frequency / 1000);
  const char *request = "GET / HTTP/1.0\r\n\r\n";
  struct http_response response;
  if (http_select(request, strlen(request), &response) != 1) {
    puts_os("OSL1 ERROR http-parser\n");
    return;
  }
  puts_os("OSL1 COUNTER advancing\nOSL1 HTTP portable-code-ready\n");
  puts_os("OSL1 ERROR network-driver-unimplemented board=pi4\n");
  for (;;) {
    if (!(uart[0x18 / 4] & (1u << 4))) {
      char c = (char)uart[0];
      if (c == '?')
        puts_os("OSL1 STATUS board=pi4 network=unimplemented\n");
    }
    __asm__ volatile("yield");
  }
}

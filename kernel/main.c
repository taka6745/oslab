#include "net.h"
#if !OSLAB_PRODUCTION
static bool last_ok = true;
static bool equal(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}
static void result(const char *command, bool ok, uint64_t elapsed) {
  puts_os("OSL1 RESULT command=");
  puts_os(command);
  field("ok", ok);
  field("elapsed_ms", elapsed);
  putc_os('\n');
  last_ok = ok;
}
static void command(char *line) {
  char *args[4];
  unsigned count = 0;
  char *p = line;
  while (*p) {
    while (*p == ' ')
      p++;
    if (!*p)
      break;
    if (count == 4) {
      result("parse", false, 0);
      return;
    }
    args[count++] = p;
    while (*p && *p != ' ')
      p++;
    if (*p)
      *p++ = 0;
  }
  if (!count)
    return;
  uint64_t start = milliseconds();
#if OSLAB_DEBUG
  if (equal(args[0], "selftest") && count == 1) {
    bool ok = memory_test();
    field("free_pages", pages_available());
    putc_os('\n');
    result("selftest", ok, milliseconds() - start);
  } else
#endif
      if (equal(args[0], "dhcp") && count == 1) {
    bool ok = net_configure();
    result("dhcp", ok, milliseconds() - start);
  }
#if !OSLAB_WEB_ONLY
  else if (equal(args[0], "resolve") && count == 2) {
    uint32_t ip;
    bool ok = net_resolve(args[1], &ip);
    result("resolve", ok, milliseconds() - start);
  } else if (equal(args[0], "http") && count == 3) {
    bool ok = net_http(args[1], args[2]);
    result("http", ok, milliseconds() - start);
  }
#endif
#if OSLAB_PROFILE
  else if (equal(args[0], "perf-reset") && count == 1) {
    perf_reset();
    puts_os("OSL1 PERF_RESET\n");
  } else if (equal(args[0], "perf") && count == 1) {
    perf_report();
  }
#endif
  else if (equal(args[0], "serve") && count == 1) {
    bool ok = net_serve();
    result("serve", ok, milliseconds() - start);
  } else if (equal(args[0], "stats") && count == 1) {
    net_stats();
    puts_os("OSL1 MEMORY");
    field("free_pages", pages_available());
    field("uptime_ms", milliseconds());
    putc_os('\n');
  }
#if OSLAB_DEBUG
  else if (equal(args[0], "fault") && count == 1)
    __asm__ volatile("ud2");
  else if (equal(args[0], "pagefault") && count == 1)
    *(volatile uint64_t *)(uintptr_t)PHYSICAL_LIMIT = 1;
  else if (equal(args[0], "hang") && count == 1) {
    puts_os("OSL1 HANG\n");
    __asm__ volatile("cli");
    for (;;)
      __asm__ volatile("hlt");
  }
#endif
  else if (equal(args[0], "exit") && count == 1) {
    puts_os("OSL1 DONE\n");
    exit_os(last_ok);
  } else {
    puts_os("OSL1 ERROR unsupported-command\n");
    last_ok = false;
  }
}
#endif
void kernel_main(void) {
#if OSLAB_PRODUCTION
  arch_init();
  memory_init();
  if (!nic_init() || !net_serve())
    panic("network-startup-failed");
  for (;;) {
    net_poll();
    idle();
  }
#else
  serial_init();
  puts_os("OSL1 BOOT kernel long64\n");
  arch_init();
  memory_init();
  puts_os("OSL1 BUILD");
  field("debug", OSLAB_DEBUG);
  putc_os('\n');
  puts_os("OSL1 MEMORY");
  field("free_pages", pages_available());
  putc_os('\n');
  bool nic = nic_init();
  puts_os(nic ? "OSL1 DEVICE network=ready\n"
              : "OSL1 DEVICE network=absent-or-failed\n");
  puts_os("OSL1 READY\n");
#if OSLAB_AUTOSERVE
  if (nic)
    (void)net_serve();
#endif
  char line[1024];
  size_t used = 0;
  bool overflow = false;
  for (;;) {
    net_poll();
    int c;
    while ((c = serial_get()) >= 0) {
      if (c == '\r')
        continue;
      if (c == '\n') {
        if (overflow) {
          puts_os("OSL1 ERROR command-too-long\n");
          last_ok = false;
        } else {
          line[used] = 0;
          command(line);
        }
        used = 0;
        overflow = false;
      } else if (c < 32 || c > 126) {
        overflow = true;
      } else if (used + 1 < sizeof(line))
        line[used++] = (char)c;
      else
        overflow = true;
    }
    idle();
  }
#endif
}

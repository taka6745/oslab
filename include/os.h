#ifndef OSLAB_OS_H
#define OSLAB_OS_H
#include "types.h"
#if OSLAB_PRODUCTION &&                                                        \
    (OSLAB_DEBUG || OSLAB_PROFILE || !OSLAB_WEB_ONLY || !OSLAB_AUTOSERVE)
#error production requires web-only autoserve with debug and profiling disabled
#endif
#define PHYSICAL_LIMIT OSLAB_RAM_LIMIT
#define PACKED __attribute__((packed))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
size_t strlen(const char *);
void serial_init(void);
void putc_os(char);
void puts_os(const char *);
void number(uint64_t, unsigned);
void field(const char *, uint64_t);
void ip_print(uint32_t);
int serial_get(void);
void arch_init(void);
bool irq_enable(unsigned);
uint64_t milliseconds(void);
void idle(void);
#if OSLAB_PROFILE
void perf_reset(void);
void perf_report(void);
#endif
void *pages_alloc(size_t);
bool pages_free(void *, size_t);
uint64_t pages_available(void);
void memory_init(void);
bool memory_test(void);
#if OSLAB_PRODUCTION
_Noreturn void halt_os(void);
#define panic(reason) halt_os()
#else
_Noreturn void panic(const char *);
#endif
_Noreturn void exit_os(bool);
#endif

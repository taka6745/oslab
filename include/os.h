#ifndef OSLAB_OS_H
#define OSLAB_OS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PHYSICAL_LIMIT 0x100000000ull
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
uint64_t milliseconds(void);
void idle(void);
void *pages_alloc(size_t);
bool pages_free(void *, size_t);
uint64_t pages_available(void);
void memory_init(void);
bool memory_test(void);
_Noreturn void panic(const char *);
_Noreturn void exit_os(bool);
#endif

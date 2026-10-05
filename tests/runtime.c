#define _DEFAULT_SOURCE
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
void *os_memcpy(void *, const void *, size_t);
static unsigned char source[1024], destination[1024];
int main(void) {
  for (size_t i = 0; i < sizeof(source); i++)
    source[i] = (unsigned char)(i * 73 + 19);
  for (size_t a = 0; a < 16; a++)
    for (size_t b = 0; b < 16; b++)
      for (size_t n = 0; n <= 257; n++) {
        for (size_t i = 0; i < sizeof(destination); i++)
          destination[i] = 0xa5;
        assert(os_memcpy(destination + 32 + a, source + b, n) ==
               destination + 32 + a);
        for (size_t i = 0; i < sizeof(destination); i++)
          assert(destination[i] == (i >= 32 + a && i - (32 + a) < n
                                        ? source[b + i - (32 + a)]
                                        : 0xa5));
      }
  size_t page = (size_t)sysconf(_SC_PAGESIZE);
  assert(page >= 4096);
  unsigned char *s =
      mmap(NULL, page * 3, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  unsigned char *d =
      mmap(NULL, page * 3, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  assert(s != MAP_FAILED && d != MAP_FAILED);
  assert(!mprotect(s + page, page, PROT_READ | PROT_WRITE));
  assert(!mprotect(d + page, page, PROT_READ | PROT_WRITE));
  for (size_t i = 0; i < page; i++)
    s[page + i] = (unsigned char)(i * 31 + 7);
  for (size_t n = 0; n <= 4096; n++) {
    unsigned char *start = d + 2 * page - n;
    assert(os_memcpy(start, s + 2 * page - n, n) == start);
    for (size_t i = 0; i < n; i++)
      assert(start[i] == s[2 * page - n + i]);
    assert(os_memcpy(d + page, s + page, n) == d + page);
    for (size_t i = 0; i < n; i++)
      assert(d[page + i] == s[page + i]);
  }
  assert(!munmap(s, page * 3) && !munmap(d, page * 3));
  puts("runtime copy alignment, tails, zero length and guard pages passed");
}

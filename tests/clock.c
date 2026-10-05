#include <assert.h>
#include <stdint.h>
#include <stdio.h>
uint64_t clock_milliseconds(uint64_t, uint64_t);
static void check(uint64_t count, uint64_t period) {
  uint64_t expected = (unsigned __int128)count * period / 1000000000000ull;
  assert(clock_milliseconds(count, period) == expected);
}
int main(void) {
  uint64_t counts[] = {0, 1, 99999, 100000, 100001, UINT64_MAX};
  uint64_t periods[] = {1, 10000000, 41666667, 99999999, 100000000};
  for (unsigned i = 0; i < sizeof(counts) / sizeof(*counts); i++)
    for (unsigned j = 0; j < sizeof(periods) / sizeof(*periods); j++)
      check(counts[i], periods[j]);
  uint64_t seed = 0x2dfe9871;
  for (unsigned i = 0; i < 100000; i++) {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    check(seed, 1 + seed % 100000000);
  }
  puts("clock exact conversion boundaries/oracle passed");
}

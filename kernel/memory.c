#include "os.h"
// One bit per physical 4 KiB page below 4 GiB, derived from BIOS E820.
#define PAGE_COUNT (PHYSICAL_LIMIT / 4096)
_Static_assert(PHYSICAL_LIMIT > 0x400000 && PHYSICAL_LIMIT <= 0x100000000ull &&
                   PHYSICAL_LIMIT % 32768 == 0,
               "physical allocator limit");
static uint8_t used[PAGE_COUNT / 8];
static uint8_t eligible[PAGE_COUNT / 8];
static size_t search_page = 1024;
static uint64_t available;
struct e820 {
  uint64_t base, length;
  uint32_t type, attributes;
} PACKED;
static bool bit(const uint8_t *map, size_t p) {
  return map[p / 8] & (1u << (p % 8));
}
static void set(uint8_t *map, size_t p, bool value) {
  if (value)
    map[p / 8] |= 1u << (p % 8);
  else
    map[p / 8] &= ~(1u << (p % 8));
}
void memory_init(void) {
  memset(used, 255, sizeof(used));
  uint16_t count = *(volatile uint16_t *)(uintptr_t)0x5000;
  if (!count || count > 64)
    panic("e820-count");
  const struct e820 *m = (const void *)(uintptr_t)0x5010;
  for (unsigned i = 0; i < count; i++)
    if (m[i].type == 1 && (m[i].attributes & 1)) {
      uint64_t end = m[i].base + m[i].length;
      if (end < m[i].base)
        panic("e820-overflow");
      if (end > PHYSICAL_LIMIT)
        end = PHYSICAL_LIMIT;
      uint64_t begin = m[i].base < 0x400000 ? 0x400000 : m[i].base;
      if (begin >= end)
        continue;
      size_t first = (begin + 4095) / 4096, last = end / 4096;
      for (size_t p = first; p < last; p++)
        if (bit(used, p)) {
          set(used, p, false);
          set(eligible, p, true);
          available++;
        }
    }
  // Reserved E820 ranges win even on firmware with overlapping records.
  for (unsigned i = 0; i < count; i++)
    if (m[i].type != 1 || !(m[i].attributes & 1)) {
      uint64_t end = m[i].base + m[i].length;
      if (end < m[i].base)
        panic("e820-overflow");
      if (end > PHYSICAL_LIMIT)
        end = PHYSICAL_LIMIT;
      if (m[i].base >= end)
        continue;
      for (size_t p = m[i].base / 4096; p < (end + 4095) / 4096; p++)
        if (bit(eligible, p)) {
          set(eligible, p, false);
          set(used, p, true);
          available--;
        }
    }
}
uint64_t pages_available(void) { return available; }
void *pages_alloc(size_t n) {
  if (!n || n > available)
    return NULL;
  for (unsigned pass = 0; pass < 2; pass++) {
    size_t first = pass ? 1024 : search_page,
           last = pass ? search_page : PAGE_COUNT, run = 0;
    for (size_t p = first; p < last; p++) {
      run = bit(used, p) ? 0 : run + 1;
      if (run == n) {
        size_t start = p + 1 - n;
        for (size_t j = start; j <= p; j++)
          set(used, j, true);
        available -= n;
        search_page = p + 1;
        void *v = (void *)(uintptr_t)(start * 4096);
        memset(v, 0, n * 4096);
        return v;
      }
    }
  }
  return NULL;
}
bool pages_free(void *v, size_t n) {
  uintptr_t a = (uintptr_t)v;
  if (a % 4096 || a < 0x400000 || a >= PHYSICAL_LIMIT || !n ||
      n > (PHYSICAL_LIMIT - a) / 4096)
    return false;
  size_t p = a / 4096;
  for (size_t i = p; i < p + n; i++)
    if (!bit(eligible, i) || !bit(used, i))
      return false;
  for (size_t i = p; i < p + n; i++)
    set(used, i, false);
  available += n;
  if (p < search_page)
    search_page = p;
  return true;
}
#if OSLAB_DEBUG
bool memory_test(void) {
  uint64_t before = available;
  uint8_t *a = pages_alloc(2), *b = pages_alloc(1);
  if (!a || !b || a == b)
    return false;
  a[0] = 7;
  a[8191] = 19;
  b[4095] = 23;
  bool ok = a[0] == 7 && a[8191] == 19 && b[4095] == 23 &&
            !pages_alloc(SIZE_MAX) && !pages_alloc(0) && !pages_free(a + 1, 1);
  ok = pages_free(a, 2) && !pages_free(a, 2) && pages_free(b, 1) && ok;
  if (!ok || available != before)
    return false;
  // Exercise real exhaustion without trusting only an oversized-allocation
  // check.
  void *all = pages_alloc((size_t)available);
  if (!all)
    return false;
  ok = !pages_alloc(1) && pages_available() == 0;
  return pages_free(all, (size_t)before) && ok && available == before;
}

#endif

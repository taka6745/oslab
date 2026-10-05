/* Authored differential execution of real response data and generic validator. */
#include "http.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
extern int readable_http_select(const char *, size_t, struct http_response *);
extern int machine_http_select(const char *, size_t, struct http_response *);
static uint64_t seed = 0x7af309189ae51d4bull;
static uint64_t next(void) {
  seed ^= seed << 13;
  seed ^= seed >> 7;
  return seed ^= seed << 17;
}
static void check(const char *p, size_t n) {
  struct http_response a = {(const char *)0x1234, 0x5678}, b = a;
  int x = readable_http_select(p, n, &a);
  int y = machine_http_select(p, n, &b);
  assert(x == y);
  assert(a.size == b.size);
  if (x == 1)
    assert(!memcmp(a.data, b.data, a.size));
  else
    assert(a.data == b.data);
}
int main(void) {
  long page = sysconf(_SC_PAGESIZE);
  assert(page > HTTP_REQUEST_LIMIT + 1);
  char *map = mmap(NULL, (size_t)page * 3, PROT_NONE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
  assert(map != MAP_FAILED);
  assert(!mprotect(map + page, (size_t)page, PROT_READ | PROT_WRITE));
  const char *cases[] = {
      "GET / HTTP/1.1\r\nHost: oslab\r\n\r\n",
      "GET /other HTTP/1.0\r\n\r\n",
      "POST / HTTP/1.1\r\nHost: a\r\n\r\n",
      "GET / HTTP/1.0\r\n\r\n", "GET / HTTP/1.1\r\n\r\n",
      "GET / HTTP/1.0\r\nContent-Length: 0\r\n\r\n",
      "GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n",
      "GET / HTTP/1.0\r\nTransfer-Encoding: chunked\r\n\r\n",
      "GET / HTTP/1.0\r\nContent-Length: 1\r\n\r\nx",
      "GET / HTTP/9.9\r\n\r\n", "GET  HTTP/1.0\r\n\r\n",
      "GET /HTTP/1.0\r\n\r\n", " / HTTP/1.0\r\n\r\n",
      "\r\n\r\n", "GET / HTTP/1.0\r\nBroken header\r\n\r\n",
      "GET / HTTP/1.1\r\nhOsT:\t a \t\r\n\r\n",
      "GET / HTTP/1.1\r\nHost: a b\r\n\r\n",
      "GET / HTTP/1.0\r\nContent-Length: 000\r\n\r\n",
      "GET / HTTP/1.0\r\nContent-Length: 0\r\ncontent-length: 0\r\n\r\n",
      "GET / HTTP/1.0\r\ntRaNsFeR-EnCoDiNg: identity\r\n\r\n",
      "GET / HTTP/1.0\r\n!#$%&'*+-.^_`|~: v\r\n\r\n"};
  char *right = map + page * 2;
  check(map, HTTP_REQUEST_LIMIT + 1); /* Reject before touching guarded input. */
  for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
    size_t len = strlen(cases[c]);
    for (size_t n = 0; n <= len; n++) {
      memcpy(right - n, cases[c], n);
      check(right - n, n);
    }
  }
  /* Both guard sides, including zero length and over-limit length. */
  for (size_t n = 0; n <= HTTP_REQUEST_LIMIT + 1; n++) {
    memset(right - n, 'x', n);
    check(right - n, n);
    memset(map + page, 'x', n);
    check(map + page, n);
  }
  /* Every byte in token, generic value and Host value grammar positions. */
  const char *grammar[] = {"GET / HTTP/1.0\r\nX: v\r\n\r\n",
                           "GET / HTTP/1.0\r\nX: Y\r\n\r\n",
                           "GET / HTTP/1.1\r\nHost: Y\r\n\r\n",
                           "GET / HTTP/1.0\r\nContent-Length: Y\r\n\r\n"};
  const size_t position[] = {16, 19, 22, 32};
  for (size_t g = 0; g < sizeof(grammar) / sizeof(grammar[0]); g++) {
    size_t n = strlen(grammar[g]);
    assert(position[g] < n);
    for (unsigned byte = 0; byte < 256; byte++) {
      memcpy(right - n, grammar[g], n);
      (right - n)[position[g]] = (char)byte;
      check(right - n, n);
    }
  }
  for (unsigned trial = 0; trial < 200000; trial++) {
    size_t c = next() % (sizeof(cases) / sizeof(cases[0]));
    size_t n = strlen(cases[c]);
    char buffer[HTTP_REQUEST_LIMIT + 1];
    memcpy(buffer, cases[c], n);
    unsigned changes = 1 + next() % 8;
    for (unsigned i = 0; i < changes; i++)
      buffer[next() % n] = (char)next();
    if (trial % 3 == 0)
      n = next() % (n + 1);
    if (trial % 7 == 0) {
      n = next() % sizeof(buffer);
      for (size_t i = 0; i < n; i++)
        buffer[i] = (char)next();
    }
    memcpy(right - n, buffer, n);
    check(right - n, n);
  }
  assert(!munmap(map, (size_t)page * 3));
  puts("Machine HTTP: response-byte oracle, guard pages and 200000 seeded mutations passed");
}

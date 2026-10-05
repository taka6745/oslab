#include "http.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  struct http_response r;
  const char *get = "GET / HTTP/1.1\r\nHost: oslab\r\n\r\n";
  for (size_t i = 0; i < strlen(get); i++)
    assert(http_select(get, i, &r) == 0);
  assert(http_select(get, strlen(get), &r) == 1);
  assert(!memcmp(r.data, "HTTP/1.0 200", 12));
  assert(r.size <= 1460);
  assert(!memcmp(r.data, "HTTP/1.0 200 \r\n", 15));
  const char *separator = NULL;
  for (size_t i = 0; i + 3 < r.size; i++)
    if (!memcmp(r.data + i, "\r\n\r\n", 4)) {
      separator = r.data + i + 4;
      break;
    }
  assert(separator);
  size_t actual = r.size - (size_t)(separator - r.data);
  const char *field = NULL;
  for (size_t i = 0; i + 15 < (size_t)(separator - r.data); i++)
    if (!memcmp(r.data + i, "Content-Length:", 15))
      field = r.data + i + 15;
  assert(field);
  size_t declared = 0;
  for (unsigned i = 0; i < 4; i++) {
    assert(field[i] >= '0' && field[i] <= '9');
    declared = declared * 10 + (size_t)(field[i] - '0');
  }
  assert(declared == actual);
  const char *bad[] = {"GET /other HTTP/1.0\r\n\r\n",
                       "POST / HTTP/1.1\r\nHost: oslab\r\n\r\n",
                       "GET / HTTP/9.9\r\n\r\n"};
  const char *status[] = {"404", "405", "400"};
  for (unsigned i = 0; i < 3; i++) {
    assert(http_select(bad[i], strlen(bad[i]), &r) == 1);
    assert(!memcmp(r.data + 9, status[i], 3));
  }
  const char *framing[] = {
      "GET / HTTP/1.1\r\n\r\n", "GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n",
      "GET / HTTP/1.0\r\nContent-Length: 1\r\n\r\nx",
      "GET / HTTP/1.0\r\nTransfer-Encoding: chunked\r\n\r\n",
      "GET / HTTP/1.0\r\nBroken header\r\n\r\n"};
  for (size_t i = 0; i < sizeof(framing) / sizeof(framing[0]); i++) {
    assert(http_select(framing[i], strlen(framing[i]), &r) == 1);
    assert(!memcmp(r.data + 9, "400", 3));
  }
  uint64_t state = 0x5eed0123456789abull;
  char fuzz[HTTP_REQUEST_LIMIT + 1];
  for (unsigned trial = 0; trial < 20000; trial++) {
    size_t length = trial % sizeof(fuzz);
    for (size_t i = 0; i < length; i++) {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      fuzz[i] = (char)state;
    }
    int status = http_select(fuzz, length, &r);
    assert(status >= -1 && status <= 1);
  }
  for (unsigned trial = 0; trial < 10000; trial++) {
    size_t length = strlen(get);
    memcpy(fuzz, get, length);
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    fuzz[state % length] = (char)(state >> 16);
    if (trial % 3 == 0)
      length = state % (length + 1);
    int status = http_select(fuzz, length, &r);
    assert(status >= -1 && status <= 1);
  }
  char data[HTTP_REQUEST_LIMIT + 1] = {0};
  assert(http_select(data, sizeof(data), &r) == -1);
  puts("HTTP regressions and 30000 seeded malformed inputs passed");
}

// Host sanitizer/seeded fuzz tests of the exact guest packet parser source.
#include "net.h"
#include <assert.h>
#include <stdio.h>
static uint64_t rng = 0x5eed0123456789abull;
static uint32_t random32(void) {
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return (uint32_t)rng;
}
static void ip_tests(void) {
  uint8_t p[64] = {0};
  struct ipv4_view ip;
  p[0] = 0x45;
  p[8] = 64;
  p[9] = 17;
  put16(p + 2, 28);
  put32(p + 12, 0xc0000201);
  put32(p + 16, 0xc6336402);
  put16(p + 10, checksum(p, 20));
  assert(ipv4_parse(p, 28, &ip) && ip.size == 8 && ip.source == 0xc0000201);
  for (size_t n = 0; n < 28; n++)
    assert(!ipv4_parse(p, n, &ip));
  p[8] ^= 1;
  assert(!ipv4_parse(p, 28, &ip));
  p[8] ^= 1;
  put16(p + 6, 0x2000);
  put16(p + 10, 0);
  put16(p + 10, checksum(p, 20));
  assert(!ipv4_parse(p, 28, &ip));
  uint8_t even[] = {0, 1, 0xf2, 3, 0xf4, 0xf5, 0xf6, 0xf7};
  assert(checksum(even, sizeof(even)) == 0x220d);
  uint8_t odd[] = {0, 1, 0xf2, 3, 0xf4, 5, 0xf6};
  assert(checksum(odd, sizeof(odd)) == 0x23f4);
  uint8_t udp[9] = {0};
  put16(udp + 4, 9);
  udp[8] = 17;
  put16(udp + 6, transport_checksum(0xc0000201, 0xc6336402, 17, udp, 9));
  assert(!transport_checksum(0xc0000201, 0xc6336402, 17, udp, 9));
  udp[8] ^= 1;
  assert(transport_checksum(0xc0000201, 0xc6336402, 17, udp, 9));
}
static void dhcp_tests(void) {
  uint8_t p[300] = {0}, mac[] = {2, 3, 4, 5, 6, 7};
  struct lease l;
  p[0] = 2;
  p[1] = 1;
  p[2] = 6;
  put32(p + 4, 123);
  memcpy(p + 28, mac, 6);
  put32(p + 236, 0x63825363);
  put32(p + 16, 0xc0000203);
  p[240] = 53;
  p[241] = 1;
  p[242] = 5;
  p[243] = 54;
  p[244] = 4;
  put32(p + 245, 0xc0000201);
  p[249] = 255;
  assert(dhcp_parse(p, 250, 123, mac, &l) && l.message == 5 &&
         l.address == 0xc0000203 && l.server == 0xc0000201);
  for (size_t n = 0; n < 250; n++)
    assert(!dhcp_parse(p, n, 123, mac, &l));
  assert(!dhcp_parse(p, 250, 124, mac, &l));
  p[28] ^= 1;
  assert(!dhcp_parse(p, 250, 123, mac, &l));
  p[28] ^= 1;
  p[249] = 53;
  p[250] = 1;
  p[251] = 5;
  p[252] = 255;
  assert(!dhcp_parse(p, 253, 123, mac, &l));
  p[244] = 255;
  assert(!dhcp_parse(p, 253, 123, mac, &l));
}
static void dns_tests(void) {
  uint8_t p[512];
  size_t len;
  uint32_t address;
  assert(dns_query(p, sizeof(p), "test.example", 17, &len));
  assert(!dns_query(p, 12, "test.example", 17, &len));
  assert(!dns_query(p, sizeof(p), "a..example", 17, &len));
  assert(!dns_query(p, sizeof(p), "bad\r\nhost", 17, &len));
  assert(dns_query(p, sizeof(p), "test.example", 17, &len));
  put16(p + 2, 0x8180);
  put16(p + 6, 1);
  p[len] = 0xc0;
  p[len + 1] = 12;
  put16(p + len + 2, 1);
  put16(p + len + 4, 1);
  put32(p + len + 6, 60);
  put16(p + len + 10, 4);
  put32(p + len + 12, 0xcb007109);
  len += 16;
  assert(dns_answer(p, len, "test.example", 17, &address) &&
         address == 0xcb007109);
  for (size_t n = 0; n < len; n++)
    assert(!dns_answer(p, n, "test.example", 17, &address));
  assert(!dns_answer(p, len, "unrelated.example", 17, &address));
  assert(!dns_answer(p, len, "test.example", 18, &address));
  p[12] = 0xc0;
  p[13] = 12;
  assert(!dns_answer(p, len, "test.example", 17, &address));
}
static void tcp_tests(void) {
  uint16_t mss;
  uint8_t p[] = {1, 2, 4, 0x05, 0xb4, 0};
  assert(tcp_mss(p, sizeof(p), &mss) && mss == 1460);
  assert(tcp_mss(p, 0, &mss) && mss == 536);
  for (size_t n = 2; n < 5; n++)
    assert(!tcp_mss(p, n, &mss));
  p[2] = 3;
  assert(!tcp_mss(p, sizeof(p), &mss));
  p[2] = 4;
  p[3] = 0;
  p[4] = 0;
  assert(!tcp_mss(p, sizeof(p), &mss));
}
int main(void) {
  tcp_tests();
  const uint64_t seed = rng;
  ip_tests();
  dhcp_tests();
  dns_tests();
  uint8_t p[2048], mac[6] = {0};
  struct ipv4_view ip;
  struct lease lease;
  uint32_t address;
  uint16_t fuzz_mss;
  for (unsigned sample = 0; sample < 100000; sample++) {
    size_t n = random32() % sizeof(p);
    for (size_t i = 0; i < n; i++)
      p[i] = random32();
    (void)tcp_mss(p, n < 40 ? n : 40, &fuzz_mss);
    (void)ipv4_parse(p, n, &ip);
    (void)dhcp_parse(p, n, random32(), mac, &lease);
    (void)dns_answer(p, n, "test.example", random32(), &address);
  }
  // Structured mutations retain outer envelopes to exercise deep parse paths.
  for (unsigned sample = 0; sample < 20000; sample++) {
    size_t n = 240 + random32() % 256;
    for (size_t i = 0; i < n; i++)
      p[i] = random32();
    p[0] = 2;
    p[1] = 1;
    p[2] = 6;
    put32(p + 4, 99);
    memcpy(p + 28, mac, 6);
    put32(p + 236, 0x63825363);
    (void)dhcp_parse(p, n, 99, mac, &lease);
    assert(dns_query(p, sizeof(p), "test.example", 17, &n));
    put16(p + 2, 0x8180);
    put16(p + 6, random32() % 32);
    size_t extra = random32() % 1000;
    for (size_t i = 0; i < extra; i++)
      p[n + i] = random32();
    (void)dns_answer(p, n + extra, "test.example", 17, &address);
    n = 20 + random32() % 1500;
    for (size_t i = 0; i < n; i++)
      p[i] = random32();
    p[0] = 0x45;
    put16(p + 2, n);
    put16(p + 6, 0x4000);
    p[8] = 64;
    put16(p + 10, 0);
    put16(p + 10, checksum(p, 20));
    assert(ipv4_parse(p, n, &ip) && ip.size == n - 20);
  }
  printf("seed=0x%016llx\n", (unsigned long long)seed);
  puts("OS packet parsers: regressions and 100000 raw and 20000 structured "
       "seeded malformed inputs passed");
  return 0;
}

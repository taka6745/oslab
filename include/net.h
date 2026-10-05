#ifndef OSLAB_NET_H
#define OSLAB_NET_H
#include "os.h"
static inline uint16_t be16(const uint8_t *p) {
  return (uint16_t)p[0] << 8 | p[1];
}
static inline uint32_t be32(const uint8_t *p) {
  return (uint32_t)be16(p) << 16 | be16(p + 2);
}
static inline void put16(uint8_t *p, uint16_t v) {
  p[0] = v >> 8;
  p[1] = v;
}
static inline void put32(uint8_t *p, uint32_t v) {
  put16(p, v >> 16);
  put16(p + 2, v);
}
uint16_t checksum(const void *, size_t);
uint16_t transport_checksum(uint32_t, uint32_t, uint8_t, const void *, size_t);
struct ipv4_view {
  uint32_t source, destination;
  uint8_t protocol;
  const uint8_t *data;
  size_t size;
};
bool ipv4_parse(const uint8_t *, size_t, struct ipv4_view *);
struct lease {
  uint32_t address, mask, router, dns, server, seconds;
  uint8_t message;
};
bool dhcp_parse(const uint8_t *, size_t, uint32_t, const uint8_t *,
                struct lease *);
bool dns_query(uint8_t *, size_t, const char *, uint16_t, size_t *);
bool dns_answer(const uint8_t *, size_t, const char *, uint16_t, uint32_t *);
bool tcp_mss(const uint8_t *, size_t, uint16_t *);
struct nic_stats {
  uint64_t tx_packets, rx_packets, tx_bytes, rx_bytes, rx_errors, tx_full,
      dropped;
};
extern struct nic_stats nic_stats;
extern uint8_t nic_mac[6];
bool nic_init(void);
bool nic_send(const uint8_t *, size_t);
void nic_poll(void (*)(const uint8_t *, size_t));
bool nic_link(void);
bool nic_pending(void);
#if OSLAB_WEB_ONLY
bool nic_recent(void);
#endif
void nic_interrupt(unsigned);
bool net_configure(void);
bool net_resolve(const char *, uint32_t *);
bool net_http(const char *, const char *);
bool net_serve(void);
void net_poll(void);
void net_stats(void);
#endif

#include "cpu.h"
#include "http.h"
#include "net.h"
static struct lease config, offered;
static bool configured, offer_ready, ack_ready, dns_ready;
static uint32_t xid, dns_ip;
static uint16_t dns_id, dns_port, ip_id;
static char dns_host[254];
static uint8_t next_mac[6];
static uint32_t next_ip;
static uint64_t arp_expires, lease_expires;
static uint8_t broadcast[6] = {255, 255, 255, 255, 255, 255};
#if !OSLAB_PRODUCTION
static uint64_t rejected;
#endif
struct connection {
  uint32_t remote, snd_una, snd_next, rcv_next;
  uint16_t local_port, remote_port, window, mss;
  bool active, established, failed, peer_fin, mac_known;
  uint8_t peer_mac[6];
  uint64_t bytes;
  uint32_t hash;
};
static struct connection tcp;
static char tcp_header[1024];
static size_t tcp_header_used;
static bool ether(const uint8_t *mac, uint16_t type, const uint8_t *data,
                  size_t n) {
  uint8_t frame[1514];
  if (n > 1500)
    return false;
  memcpy(frame, mac, 6);
  memcpy(frame + 6, nic_mac, 6);
  put16(frame + 12, type);
  memcpy(frame + 14, data, n);
  size_t len = 14 + n;
  if (len < 60) {
    memset(frame + len, 0, 60 - len);
    len = 60;
  }
  return nic_send(frame, len);
}
static void arp_send(uint16_t op, const uint8_t *mac, uint32_t destination) {
  uint8_t p[28] = {0};
  put16(p, 1);
  put16(p + 2, 0x800);
  p[4] = 6;
  p[5] = 4;
  put16(p + 6, op);
  memcpy(p + 8, nic_mac, 6);
  put32(p + 14, config.address);
  if (op == 2)
    memcpy(p + 18, mac, 6);
  put32(p + 24, destination);
  (void)ether(mac, 0x806, p, sizeof(p));
}
static void receive(const uint8_t *, size_t);
static void server_poll(void);
static bool receiving, polling_server;
void net_poll(void) {
  if (receiving)
    return;
  receiving = true;
  nic_poll(receive);
  receiving = false;
  if (!polling_server) {
    polling_server = true;
    server_poll();
    polling_server = false;
  }
}
static bool arp_resolve(uint32_t ip, uint8_t *mac) {
  uint32_t target =
      (ip & config.mask) == (config.address & config.mask) ? ip : config.router;
  if (!target)
    return false;
  if (target == next_ip && milliseconds() < arp_expires) {
    memcpy(mac, next_mac, 6);
    return true;
  }
  next_ip = target;
  arp_expires = 0;
  // Never wait recursively on our own RX descriptor; retry after ARP.
  if (receiving) {
    arp_send(1, broadcast, target);
    return false;
  }
  for (unsigned retry = 0; retry < 4; retry++) {
    next_ip = target;
    arp_expires = 0;
    arp_send(1, broadcast, target);
    uint64_t deadline = milliseconds() + 500;
    while (milliseconds() < deadline) {
      net_poll();
      if (next_ip == target && milliseconds() < arp_expires) {
        memcpy(mac, next_mac, 6);
        return true;
      }
      idle();
    }
  }
  return false;
}
static bool ip_send_to(uint32_t source, uint32_t destination, uint8_t proto,
                       const uint8_t *p, size_t n, const uint8_t *known_mac) {
  uint8_t data[1500], mac[6];
  if (n > 1480)
    return false;
  if (known_mac)
    memcpy(mac, known_mac, 6);
  else if (destination == 0xffffffff)
    memcpy(mac, broadcast, 6);
  else if (!configured || !arp_resolve(destination, mac))
    return false;
  memset(data, 0, 20);
  data[0] = 0x45;
  put16(data + 2, 20 + n);
  put16(data + 4, ++ip_id);
  put16(data + 6, 0x4000);
  data[8] = 64;
  data[9] = proto;
  put32(data + 12, source);
  put32(data + 16, destination);
  put16(data + 10, checksum(data, 20));
  memcpy(data + 20, p, n);
  return ether(mac, 0x800, data, n + 20);
}
static bool ip_send(uint32_t source, uint32_t destination, uint8_t proto,
                    const uint8_t *p, size_t n) {
  return ip_send_to(source, destination, proto, p, n, NULL);
}
static bool udp_send(uint32_t source, uint32_t dst, uint16_t sport,
                     uint16_t dport, const uint8_t *p, size_t n) {
  uint8_t data[1480];
  if (n > 1472)
    return false;
  put16(data, sport);
  put16(data + 2, dport);
  put16(data + 4, n + 8);
  put16(data + 6, 0);
  memcpy(data + 8, p, n);
  uint16_t c = transport_checksum(source, dst, 17, data, n + 8);
  put16(data + 6, c ? c : 65535);
  return ip_send(source, dst, 17, data, n + 8);
}
static bool dhcp_send(uint8_t kind) {
  uint8_t p[300] = {0};
  p[0] = 1;
  p[1] = 1;
  p[2] = 6;
  put32(p + 4, xid);
  put16(p + 10, 0x8000);
  memcpy(p + 28, nic_mac, 6);
  put32(p + 236, 0x63825363);
  size_t i = 240;
  p[i++] = 53;
  p[i++] = 1;
  p[i++] = kind;
  p[i++] = 61;
  p[i++] = 7;
  p[i++] = 1;
  memcpy(p + i, nic_mac, 6);
  i += 6;
  if (kind == 3) {
    p[i++] = 50;
    p[i++] = 4;
    put32(p + i, offered.address);
    i += 4;
    p[i++] = 54;
    p[i++] = 4;
    put32(p + i, offered.server);
    i += 4;
  }
  p[i++] = 55;
  p[i++] = 4;
  p[i++] = 1;
  p[i++] = 3;
  p[i++] = 6;
  p[i++] = 51;
  p[i++] = 255;
  return udp_send(0, 0xffffffff, 68, 67, p, i);
}
bool net_configure(void) {
  if (!nic_link())
    return false;
  if (configured && milliseconds() < lease_expires)
    return true;
  configured = false;
  offer_ready = false;
  ack_ready = false;
  memset(&offered, 0, sizeof(offered));
  xid = (uint32_t)cycles() ^ be32(nic_mac + 2);
  if (!xid)
    xid = 1;
  for (unsigned attempt = 0; attempt < 4; attempt++) {
    if (!dhcp_send(offer_ready ? 3 : 1))
      return false;
    uint64_t deadline = milliseconds() + 1000;
    while (milliseconds() < deadline) {
      net_poll();
      if (ack_ready)
        goto ready;
      if (offer_ready)
        break;
      idle();
    }
    if (offer_ready) {
      if (!dhcp_send(3))
        return false;
      deadline = milliseconds() + 1500;
      while (milliseconds() < deadline) {
        net_poll();
        if (ack_ready)
          goto ready;
        idle();
      }
    }
  }
  return false;
ready:
  if (!config.address || !config.mask || !config.seconds)
    return false;
  uint32_t inverse = ~config.mask;
  if (inverse & (inverse + 1))
    return false;
  if (config.router &&
      (config.router & config.mask) != (config.address & config.mask))
    return false;
  configured = true;
  next_ip = 0;
  arp_expires = 0;
  lease_expires = milliseconds() + (uint64_t)config.seconds * 1000;
#if !OSLAB_PRODUCTION
  puts_os("OSL1 DHCP address=");
  ip_print(config.address);
  puts_os(" router=");
  ip_print(config.router);
  puts_os(" dns=");
  ip_print(config.dns);
  field("lease_seconds", config.seconds);
  putc_os('\n');
#endif
  return true;
}
bool net_resolve(const char *host, uint32_t *address) {
  if (!net_configure() || !config.dns)
    return false;
  size_t h = strlen(host);
  if (h >= sizeof(dns_host))
    return false;
  memcpy(dns_host, host, h + 1);
  dns_ready = false;
  dns_id = (uint16_t)cycles();
  dns_port = 49152 + (dns_id % 16000);
  uint8_t p[512];
  size_t n;
  if (!dns_query(p, sizeof(p), host, dns_id, &n))
    return false;
  for (unsigned retry = 0; retry < 4; retry++) {
    if (!udp_send(config.address, config.dns, dns_port, 53, p, n))
      return false;
    uint64_t deadline = milliseconds() + 2000;
    while (milliseconds() < deadline) {
      net_poll();
      if (dns_ready) {
        *address = dns_ip;
        puts_os("OSL1 DNS host=");
        puts_os(host);
        puts_os(" address=");
        ip_print(dns_ip);
        putc_os('\n');
        return true;
      }
      idle();
    }
  }
  return false;
}
static bool connection_send(struct connection *c, uint8_t flags, uint32_t seq,
                            const uint8_t *data, size_t n) {
  uint8_t p[1480];
  size_t h = (flags & 2) ? 24 : 20;
  if (n > sizeof(p) - h)
    return false;
  memset(p, 0, h);
  put16(p, c->local_port);
  put16(p + 2, c->remote_port);
  put32(p + 4, seq);
  put32(p + 8, c->rcv_next);
  p[12] = (uint8_t)((h / 4) << 4);
  p[13] = flags;
  put16(p + 14, 32768);
  if (flags & 2) {
    p[20] = 2;
    p[21] = 4;
    put16(p + 22, 1460);
  }
  if (n)
    memcpy(p + h, data, n);
  put16(p + 16, transport_checksum(config.address, c->remote, 6, p, h + n));
  return ip_send_to(config.address, c->remote, 6, p, h + n,
                    c->mac_known ? c->peer_mac : NULL);
}
static bool tcp_send(uint8_t flags, uint32_t seq, const uint8_t *data,
                     size_t n) {
  return connection_send(&tcp, flags, seq, data, n);
}
static bool seq_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }
#if !OSLAB_WEB_ONLY
static void tcp_receive(const struct ipv4_view *ip) {
  const uint8_t *p = ip->data;
  size_t n = ip->size;
  if (!tcp.active || ip->source != tcp.remote || n < 20 ||
      be16(p) != tcp.remote_port || be16(p + 2) != tcp.local_port ||
      transport_checksum(ip->source, ip->destination, 6, p, n))
    return;
  size_t h = (p[12] >> 4) * 4;
  if (h < 20 || h > n)
    return;
  uint32_t seq = be32(p + 4), ack = be32(p + 8);
  uint8_t flags = p[13];
  if (!tcp.established) {
    if ((flags & 0x14) == 0x14 && ack == tcp.snd_next) {
      tcp.failed = true;
      return;
    }
    if ((flags & 0x12) != 0x12 || ack != tcp.snd_next)
      return;
    if (!tcp_mss(p + 20, h - 20, &tcp.mss))
      return;
    tcp.rcv_next = seq + 1;
    tcp.snd_una = ack;
    tcp.window = be16(p + 14);
    tcp.established = true;
    (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
    seq++; // A SYN consumes one sequence number if it also carries payload.
  } else {
    if (flags & 4) {
      if (seq == tcp.rcv_next)
        tcp.failed = true;
      else
        (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
      return;
    }
    if (flags & 2) {
      (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
      return;
    }
    if (!(flags & 0x10) || seq_before(ack, tcp.snd_una) ||
        seq_before(tcp.snd_next, ack))
      return;
    tcp.snd_una = ack;
    tcp.window = be16(p + 14);
  }
  size_t size = n - h;
  const uint8_t *payload = p + h;
  if (seq_before(seq, tcp.rcv_next)) {
    uint32_t overlap = tcp.rcv_next - seq;
    if (overlap > size) {
      (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
      return;
    }
    payload += overlap;
    size -= overlap;
    seq += overlap;
  }
  if (seq != tcp.rcv_next) {
    (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
    return;
  }
  if (size) {
    if (tcp.bytes + size > 1024 * 1024) {
      tcp.failed = true;
      return;
    }
    for (size_t i = 0; i < size; i++) {
      tcp.hash = (tcp.hash ^ payload[i]) * 16777619;
      if (tcp_header_used + 1 < sizeof(tcp_header))
        tcp_header[tcp_header_used++] = (char)payload[i];
    }
    tcp.bytes += size;
    tcp.rcv_next += size;
  }
  if ((flags & 1) && !tcp.peer_fin) {
    tcp.rcv_next++;
    tcp.peer_fin = true;
  }
  if (size || (flags & 1))
    (void)tcp_send(0x10, tcp.snd_next, NULL, 0);
}
#endif
static struct connection server;
static bool listening, closing;
static char request[HTTP_REQUEST_LIMIT];
static size_t request_size, response_offset, pending_size;
static struct http_response response;
static uint64_t server_deadline, retry_at;
static unsigned retries;
static uint32_t pending_seq;
static uint8_t pending_flags;
static void server_transmit(uint8_t flags, const uint8_t *data, size_t n) {
  pending_seq = server.snd_next;
  pending_size = n;
  pending_flags = flags;
  server.snd_next += n + !!(flags & 3);
  retries = 0;
  retry_at = milliseconds() + 250;
  (void)connection_send(&server, flags, pending_seq, data, n);
}
bool net_serve(void) {
  if (!net_configure())
    return false;
  listening = true;
#if !OSLAB_PRODUCTION
  puts_os("OSL1 SERVING port=80 address=");
  ip_print(config.address);
  putc_os('\n');
#endif
  return true;
}
static void server_receive(const struct ipv4_view *ip, const uint8_t *mac) {
  const uint8_t *p = ip->data;
  size_t n = ip->size;
  if (!listening || !configured || milliseconds() >= lease_expires || n < 20 ||
      be16(p + 2) != 80 || ip->destination != config.address ||
      transport_checksum(ip->source, ip->destination, 6, p, n))
    return;
  size_t h = (p[12] >> 4) * 4;
  if (h < 20 || h > n)
    return;
  uint32_t seq = be32(p + 4), ack = be32(p + 8);
  uint8_t flags = p[13];
  if (!server.active) {
    if ((flags & 0x17) != 2 || n != h)
      return;
    uint16_t mss;
    if (!tcp_mss(p + 20, h - 20, &mss))
      return;
    // Preserve the handshake next hop across unrelated ARP lookups.
    if (mac[0] & 1)
      return;
    memset(&server, 0, sizeof(server));
    memcpy(server.peer_mac, mac, 6);
    server.mac_known = true;
    server.active = true;
    server.remote = ip->source;
    server.remote_port = be16(p);
    server.local_port = 80;
    server.mss = mss;
    server.window = be16(p + 14);
    server.rcv_next = seq + 1;
    server.snd_una = server.snd_next = (uint32_t)cycles();
    request_size = response_offset = 0;
    response = (struct http_response){0};
    closing = false;
    server_deadline = milliseconds() + 10000;
    server_transmit(0x12, NULL, 0);
    return;
  }
  if (server.remote != ip->source || server.remote_port != be16(p))
    return;
  if (flags & 4) {
    if (seq == server.rcv_next)
      server.active = false;
    return;
  }
  if (flags & 2) {
    if (!server.established && seq + 1 == server.rcv_next)
      (void)connection_send(&server, 0x12, pending_seq, NULL, 0);
    return;
  }
  if (!(flags & 0x10) || seq_before(ack, server.snd_una) ||
      seq_before(server.snd_next, ack))
    return;
  if (!server.established) {
    if (ack != server.snd_next || seq != server.rcv_next)
      return;
    server.established = true;
  }
  server.snd_una = ack;
  server.window = be16(p + 14);
  size_t size = n - h;
  const uint8_t *payload = p + h;
  if (seq_before(seq, server.rcv_next)) {
    uint32_t overlap = server.rcv_next - seq;
    if (overlap > size) {
      (void)connection_send(&server, 0x10, server.snd_next, NULL, 0);
      return;
    }
    size -= overlap;
    payload += overlap;
    seq += overlap;
  }
  if (seq != server.rcv_next) {
    (void)connection_send(&server, 0x10, server.snd_next, NULL, 0);
    return;
  }
  if (size) {
    if (response.data || size > sizeof(request) - request_size) {
      (void)connection_send(&server, 0x14, server.snd_next, NULL, 0);
      server.active = false;
      return;
    }
    memcpy(request + request_size, payload, size);
    request_size += size;
    server.rcv_next += size;
    if (http_select(request, request_size, &response) < 0)
      server.active = false;
  }
  if ((flags & 1) && !server.peer_fin) {
    server.rcv_next++;
    server.peer_fin = true;
  }
  // The next poll sends an ACK with the selected response. Keep standalone
  // ACKs for incomplete requests, closed windows and outstanding segments.
  if ((size || (flags & 1)) &&
      !(response.data && server.snd_una == server.snd_next && server.window &&
        !closing))
    (void)connection_send(&server, 0x10, server.snd_next, NULL, 0);
  // Release now so the next frame in this batch can open a connection.
  if (closing && server.peer_fin && server.snd_una == server.snd_next)
    server.active = false;
}
static void server_poll(void) {
  if (!server.active)
    return;
  uint64_t now = milliseconds();
  if (now >= server_deadline || now >= lease_expires || !nic_link()) {
    if (now < lease_expires && nic_link())
      (void)connection_send(&server, 0x14, server.snd_next, NULL, 0);
    server.active = false;
    return;
  }
  if (server.snd_una != server.snd_next) {
    if (now >= retry_at) {
      if (++retries > 5) {
        server.active = false;
        return;
      }
      retry_at = now + 250;
      const uint8_t *data =
          pending_size ? (const uint8_t *)response.data + response_offset
                       : NULL;
      (void)connection_send(&server, pending_flags, pending_seq, data,
                            pending_size);
    }
    return;
  }
  if (pending_size) {
    response_offset += pending_size;
    pending_size = 0;
  }
  if (!server.established || !response.data)
    return;
  if (response_offset < response.size) {
    size_t n = response.size - response_offset;
    if (n > server.mss)
      n = server.mss;
    if (n > server.window)
      n = server.window;
    if (n) {
      uint8_t flags = 0x18;
      // FIN consumes sequence space too; do not put it outside the window.
      if (response_offset + n == response.size && n < server.window) {
        flags |= 1;
        closing = true;
      }
      server_transmit(flags, (const uint8_t *)response.data + response_offset,
                      n);
    }
  } else if (!closing && server.window) {
    closing = true;
    server_transmit(0x11, NULL, 0);
  } else if (server.peer_fin)
    server.active = false;
}
bool net_http(const char *host, const char *path) {
  if (path[0] != '/' || strlen(path) > 512)
    return false;
  for (size_t i = 0; path[i]; i++)
    if ((uint8_t)path[i] < 33 || (uint8_t)path[i] > 126)
      return false;
  uint32_t ip;
  if (!net_resolve(host, &ip))
    return false;
  memset(&tcp, 0, sizeof(tcp));
  tcp_header_used = 0;
  tcp.active = true;
  tcp.remote = ip;
  tcp.remote_port = 80;
  tcp.local_port = 49152 + ((uint16_t)cycles() % 16000);
  tcp.snd_next = (uint32_t)cycles();
  tcp.snd_una = tcp.snd_next;
  tcp.hash = 2166136261;
  uint32_t initial = tcp.snd_next++;
  bool ok = false;
  uint64_t begin = milliseconds();
  for (unsigned retry = 0; retry < 5 && !tcp.established && !tcp.failed;
       retry++) {
    if (!tcp_send(2, initial, NULL, 0))
      goto done;
    uint64_t deadline = milliseconds() + 1000;
    while (milliseconds() < deadline && !tcp.established && !tcp.failed) {
      net_poll();
      idle();
    }
  }
  if (!tcp.established || tcp.failed)
    goto done;
  char request[1024];
  size_t len = 0;
  const char *parts[] = {
      "GET ", path, " HTTP/1.0\r\nHost: ", host,
      "\r\nConnection: close\r\nUser-Agent: oslab/0.1\r\n\r\n"};
  for (unsigned i = 0; i < ARRAY_SIZE(parts); i++) {
    size_t n = strlen(parts[i]);
    if (len + n > sizeof(request))
      goto done;
    memcpy(request + len, parts[i], n);
    len += n;
  }
  for (size_t offset = 0; offset < len;) {
    size_t chunk = len - offset;
    if (chunk > tcp.mss)
      chunk = tcp.mss;
    if (tcp.window < chunk)
      goto done;
    uint32_t data_seq = tcp.snd_next;
    tcp.snd_next += chunk;
    for (unsigned retry = 0;
         retry < 5 && tcp.snd_una != tcp.snd_next && !tcp.failed; retry++) {
      if (!tcp_send(0x18, data_seq, (const uint8_t *)request + offset, chunk))
        goto done;
      uint64_t deadline = milliseconds() + 1000;
      while (milliseconds() < deadline && tcp.snd_una != tcp.snd_next &&
             !tcp.failed) {
        net_poll();
        idle();
      }
    }
    if (tcp.snd_una != tcp.snd_next || tcp.failed)
      goto done;
    offset += chunk;
  }
  {
    uint64_t deadline = milliseconds() + 15000;
    while (milliseconds() < deadline && !tcp.peer_fin && !tcp.failed) {
      net_poll();
      idle();
    }
  }
  if (!tcp.peer_fin || tcp.failed || tcp_header_used < 12 ||
      memcmp(tcp_header, "HTTP/1.", 7) || tcp_header[8] != ' ' ||
      tcp_header[9] < '1' || tcp_header[9] > '5' || tcp_header[10] < '0' ||
      tcp_header[10] > '9' || tcp_header[11] < '0' || tcp_header[11] > '9')
    goto done;
  // Close actively after peer FIN and wait for our own FIN acknowledgement.
  {
    uint32_t fin_seq = tcp.snd_next++;
    for (unsigned retry = 0; retry < 4 && tcp.snd_una != tcp.snd_next;
         retry++) {
      if (!tcp_send(0x11, fin_seq, NULL, 0))
        goto done;
      uint64_t deadline = milliseconds() + 500;
      while (milliseconds() < deadline && tcp.snd_una != tcp.snd_next &&
             !tcp.failed) {
        net_poll();
        idle();
      }
    }
    if (tcp.snd_una != tcp.snd_next || tcp.failed)
      goto done;
  }
  puts_os("OSL1 HTTP host=");
  puts_os(host);
  puts_os(" status=");
  for (unsigned i = 9; i < 12; i++)
    putc_os(tcp_header[i]);
  field("bytes", tcp.bytes);
  field("fnv1a", tcp.hash);
  field("elapsed_ms", milliseconds() - begin);
  putc_os('\n');
  puts_os("OSL1 HTTP_HEAD hex=");
  size_t dump = tcp_header_used < 256 ? tcp_header_used : 256;
  for (size_t i = 0; i < dump; i++) {
    uint8_t c = tcp_header[i];
    putc_os("0123456789abcdef"[c >> 4]);
    putc_os("0123456789abcdef"[c & 15]);
  }
  putc_os('\n');
  ok = true;
done:
  if (!ok) {
    if (tcp.established && !tcp.failed)
      (void)tcp_send(0x14, tcp.snd_next, NULL, 0);
    puts_os("OSL1 NET_ERROR operation=http\n");
  }
  tcp.active = false;
  return ok;
}
static void receive(const uint8_t *frame, size_t size) {
  if (size < 14)
    return;
  if (memcmp(frame, nic_mac, 6) && memcmp(frame, broadcast, 6))
    return;
  uint16_t type = be16(frame + 12);
  const uint8_t *p = frame + 14;
  size -= 14;
  if (type == 0x806) {
    if (size < 28 || be16(p) != 1 || be16(p + 2) != 0x800 || p[4] != 6 ||
        p[5] != 4 || memcmp(frame + 6, p + 8, 6))
      goto bad;
    uint32_t source = be32(p + 14), dest = be32(p + 24);
    uint16_t op = be16(p + 6);
    if (configured && op == 1 && dest == config.address)
      arp_send(2, p + 8, source);
    if (configured && op == 2 && source == next_ip && dest == config.address &&
        !memcmp(p + 18, nic_mac, 6)) {
      memcpy(next_mac, p + 8, 6);
      arp_expires = milliseconds() + 60000;
    }
    return;
  }
  if (type != 0x800)
    return;
  struct ipv4_view ip;
  if (!ipv4_parse(p, size, &ip))
    goto bad;
  if (configured && ip.destination != config.address &&
      ip.destination != 0xffffffff)
    return;
  if (ip.protocol == 17) {
    p = ip.data;
    size = ip.size;
    if (size < 8 || be16(p + 4) != size)
      goto bad;
    if (be16(p + 6) &&
        transport_checksum(ip.source, ip.destination, 17, p, size))
      goto bad;
    uint16_t sport = be16(p), dport = be16(p + 2);
    p += 8;
    size -= 8;
    if (sport == 67 && dport == 68 && !configured) {
      struct lease lease;
      if (!dhcp_parse(p, size, xid, nic_mac, &lease))
        goto bad;
      if (lease.message == 2 && lease.address && lease.server) {
        offered = lease;
        offer_ready = true;
      } else if (lease.message == 5 && offer_ready &&
                 lease.server == offered.server &&
                 lease.address == offered.address) {
        config = lease;
        ack_ready = true;
      } else if (lease.message == 6) {
        offer_ready = false;
        ack_ready = false;
      }
    }
#if !OSLAB_WEB_ONLY
    else if (configured && ip.source == config.dns && sport == 53 &&
             dport == dns_port) {
      if (dns_answer(p, size, dns_host, dns_id, &dns_ip))
        dns_ready = true;
      else
        goto bad;
    }
#endif
  } else if (ip.protocol == 6 && configured) {
#if !OSLAB_WEB_ONLY
    tcp_receive(&ip);
#endif
    server_receive(&ip, frame + 6);
  }
#if !OSLAB_PRODUCTION
  else if (ip.protocol == 1 && configured) {
    p = ip.data;
    size = ip.size;
    if (size < 8 || checksum(p, size))
      goto bad;
    if (p[0] == 8 && p[1] == 0 && size <= 1480) {
      uint8_t reply[1480];
      memcpy(reply, p, size);
      reply[0] = 0;
      put16(reply + 2, 0);
      put16(reply + 2, checksum(reply, size));
      (void)ip_send(config.address, ip.source, 1, reply, size);
    }
  }
#endif
  return;
bad:
#if !OSLAB_PRODUCTION
  rejected++;
#endif
#if !OSLAB_PRODUCTION
  nic_stats.dropped++;
#endif
  return;
}
#if !OSLAB_PRODUCTION
void net_stats(void) {
  puts_os("OSL1 NET_STATS");
  field("link", nic_link());
  field("configured", configured && milliseconds() < lease_expires);
  field("tx_packets", nic_stats.tx_packets);
  field("rx_packets", nic_stats.rx_packets);
  field("tx_bytes", nic_stats.tx_bytes);
  field("rx_bytes", nic_stats.rx_bytes);
  field("rx_errors", nic_stats.rx_errors);
  field("tx_full", nic_stats.tx_full);
  field("rejected", rejected);
  putc_os('\n');
}

#endif

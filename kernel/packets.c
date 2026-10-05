#include "net.h"
static uint32_t sum_bytes(uint32_t sum, const uint8_t *p, size_t n) {
  while (n >= 2) {
    sum += be16(p);
    p += 2;
    n -= 2;
  }
  if (n)
    sum += (uint16_t)p[0] << 8;
  return sum;
}
static uint16_t fold(uint32_t sum) {
  while (sum >> 16)
    sum = (sum & 65535) + (sum >> 16);
  return (uint16_t)~sum;
}
uint16_t checksum(const void *p, size_t n) { return fold(sum_bytes(0, p, n)); }
uint16_t transport_checksum(uint32_t src, uint32_t dst, uint8_t proto,
                            const void *data, size_t size) {
  if (size > 65535)
    return 1;
  return fold(sum_bytes((src >> 16) + (src & 65535) + (dst >> 16) +
                            (dst & 65535) + proto + (uint16_t)size,
                        data, size));
}
bool ipv4_parse(const uint8_t *p, size_t n, struct ipv4_view *v) {
  if (n < 20 || (p[0] >> 4) != 4)
    return false;
  size_t h = (p[0] & 15) * 4, total = be16(p + 2);
  if (h < 20 || h > n || total < h || total > n || !p[8] || checksum(p, h) ||
      (be16(p + 6) & 0xbfff))
    return false;
  *v = (struct ipv4_view){be32(p + 12), be32(p + 16), p[9], p + h, total - h};
  return true;
}
bool dhcp_parse(const uint8_t *p, size_t n, uint32_t xid, const uint8_t *mac,
                struct lease *result) {
  if (n < 240 || p[0] != 2 || p[1] != 1 || p[2] != 6 || be32(p + 4) != xid ||
      memcmp(p + 28, mac, 6) || be32(p + 236) != 0x63825363)
    return false;
  struct lease out = {.address = be32(p + 16)};
  uint32_t seen = 0;
  bool end = false;
  for (size_t i = 240; i < n;) {
    uint8_t type = p[i++];
    if (!type)
      continue;
    if (type == 255) {
      end = true;
      break;
    }
    if (i == n)
      return false;
    size_t len = p[i++];
    if (len > n - i)
      return false;
    uint32_t key = type == 53   ? 1
                   : type == 1  ? 2
                   : type == 3  ? 4
                   : type == 6  ? 8
                   : type == 54 ? 16
                   : type == 51 ? 32
                                : 0;
    if (key && (seen & key))
      return false;
    seen |= key;
    if (type == 53) {
      if (len != 1)
        return false;
      out.message = p[i];
    } else if (type == 1) {
      if (len != 4)
        return false;
      out.mask = be32(p + i);
    } else if (type == 3 || type == 6) {
      if (len < 4 || len % 4)
        return false;
      if (type == 3)
        out.router = be32(p + i);
      else
        out.dns = be32(p + i);
    } else if (type == 54 || type == 51) {
      if (len != 4)
        return false;
      if (type == 54)
        out.server = be32(p + i);
      else
        out.seconds = be32(p + i);
    }
    i += len;
  }
  if (!end || !(seen & 1))
    return false;
  *result = out;
  return true;
}
static bool dns_name(const uint8_t *p, size_t n, size_t *position, char *name) {
  size_t i = *position, out = 0, consumed = 0;
  bool jumped = false;
  for (unsigned hops = 0; hops < 128; hops++) {
    if (i >= n)
      return false;
    uint8_t len = p[i++];
    if (!jumped)
      consumed++;
    if (!len) {
      if (out)
        out--;
      name[out] = 0;
      *position += consumed;
      return true;
    }
    if ((len & 0xc0) == 0xc0) {
      if (i >= n)
        return false;
      size_t target = ((len & 63) << 8) | p[i++];
      if (!jumped)
        consumed++;
      if (target >= n)
        return false;
      jumped = true;
      i = target;
      continue;
    }
    if (len & 0xc0 || len > 63 || len > n - i || out + len + 1 > 254)
      return false;
    for (unsigned j = 0; j < len; j++) {
      uint8_t c = p[i++];
      if (c < 33 || c > 126 || c == '.')
        return false;
      name[out++] = (char)(c >= 'A' && c <= 'Z' ? c + 32 : c);
    }
    name[out++] = '.';
    if (!jumped)
      consumed += len;
  }
  return false;
}
bool dns_query(uint8_t *p, size_t n, const char *host, uint16_t id,
               size_t *length) {
  size_t size = strlen(host);
  if (!size || size > 253 || n < 18 + size)
    return false;
  memset(p, 0, 12);
  put16(p, id);
  put16(p + 2, 0x100);
  put16(p + 4, 1);
  size_t pos = 12, start = 0;
  for (size_t i = 0; i <= size; i++)
    if (host[i] == '.' || !host[i]) {
      size_t len = i - start;
      if (!len || len > 63)
        return false;
      p[pos++] = (uint8_t)len;
      for (size_t j = start; j < i; j++) {
        uint8_t c = host[j];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-'))
          return false;
        p[pos++] = c;
      }
      start = i + 1;
    }
  p[pos++] = 0;
  put16(p + pos, 1);
  put16(p + pos + 2, 1);
  pos += 4;
  *length = pos;
  return true;
}
bool dns_answer(const uint8_t *p, size_t n, const char *host, uint16_t id,
                uint32_t *address) {
  if (n < 12 || be16(p) != id || (be16(p + 2) & 0xfa0f) != 0x8000 ||
      be16(p + 4) != 1)
    return false;
  size_t pos = 12;
  char q[255];
  if (!dns_name(p, n, &pos, q) || pos + 4 > n || be16(p + pos) != 1 ||
      be16(p + pos + 2) != 1)
    return false;
  size_t hl = strlen(host);
  if (strlen(q) != hl)
    return false;
  for (size_t i = 0; i < hl; i++) {
    char c = host[i];
    if (c >= 'A' && c <= 'Z')
      c += 32;
    if (q[i] != c)
      return false;
  }
  pos += 4;
  unsigned count = be16(p + 6);
  if (count > 128)
    return false;
  // Reject unrelated additional-section data.
  char wanted[255];
  memcpy(wanted, q, hl + 1);
  struct record {
    char owner[255], target[255];
    uint32_t ip;
    uint16_t type;
  } records[32];
  unsigned used = 0;
  if (count > ARRAY_SIZE(records))
    return false;
  for (unsigned i = 0; i < count; i++) {
    struct record *r = &records[used++];
    memset(r, 0, sizeof(*r));
    if (!dns_name(p, n, &pos, r->owner) || pos + 10 > n)
      return false;
    r->type = be16(p + pos);
    uint16_t cls = be16(p + pos + 2), len = be16(p + pos + 8);
    pos += 10;
    if (len > n - pos)
      return false;
    if (cls != 1)
      r->type = 0;
    if (r->type == 1) {
      if (len != 4)
        return false;
      r->ip = be32(p + pos);
    } else if (r->type == 5) {
      size_t end = pos;
      if (!dns_name(p, n, &end, r->target) || end != pos + len)
        return false;
    }
    pos += len;
  }
  for (unsigned hop = 0; hop < 32; hop++) {
    bool found = false;
    for (unsigned i = 0; i < used; i++)
      if (!memcmp(records[i].owner, wanted, strlen(wanted) + 1)) {
        if (records[i].type == 1 && records[i].ip) {
          *address = records[i].ip;
          return true;
        }
        if (records[i].type == 5) {
          memcpy(wanted, records[i].target, strlen(records[i].target) + 1);
          found = true;
          break;
        }
      }
    if (!found)
      return false;
  }
  return false;
}

// IPv4's default peer MSS is 536 when SYN carries no MSS option (RFC 9293).
bool tcp_mss(const uint8_t *p, size_t n, uint16_t *mss) {
  uint16_t value = 536;
  bool seen = false;
  if (n > 40)
    return false;
  for (size_t i = 0; i < n;) {
    uint8_t kind = p[i++];
    if (!kind)
      break;
    if (kind == 1)
      continue;
    if (i == n)
      return false;
    uint8_t len = p[i++];
    if (len < 2 || len - 2 > n - i)
      return false;
    if (kind == 2) {
      if (seen || len != 4)
        return false;
      value = be16(p + i);
      seen = true;
      if (!value)
        return false;
      if (value > 1460)
        value = 1460;
    }
    i += len - 2;
  }
  *mss = value;
  return true;
}

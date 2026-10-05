#include "net.h"
#include "x86.h"
// Intel 82540EM/82574 legacy 16-byte DMA descriptors.
#define RING OSLAB_NIC_RING
_Static_assert(
    RING >= 8 && RING <= 256 && (RING & (RING - 1)) == 0,
    "e1000 ring must be a power of two, at least 128 descriptor bytes");
#define BUFFER 2048
#define TX_BUFFERS OSLAB_TX_BUFFERS
_Static_assert(TX_BUFFERS >= 2 && TX_BUFFERS <= RING && RING % TX_BUFFERS == 0,
               "TX buffer pool");
static unsigned tx_owner[TX_BUFFERS];
struct rx_desc {
  uint64_t address;
  uint16_t length, checksum;
  uint8_t status, errors;
  uint16_t special;
} PACKED;
struct tx_desc {
  uint64_t address;
  uint16_t length;
  uint8_t cso, command, status, css;
  uint16_t special;
} PACKED;
_Static_assert(sizeof(struct rx_desc) == 16, "RX layout");
_Static_assert(sizeof(struct tx_desc) == 16, "TX layout");
static volatile uint32_t *mmio;
static volatile struct rx_desc *rx;
static volatile struct tx_desc *tx;
static uint8_t *rx_data, *tx_data;
static unsigned rx_head, tx_tail;
#if OSLAB_WEB_ONLY
static uint64_t last_receive;
static bool received;
#endif
static unsigned nic_irq_line = 255;
#if !OSLAB_PRODUCTION
struct nic_stats nic_stats;
#endif
uint8_t nic_mac[6];
static uint32_t pci_read(unsigned b, unsigned d, unsigned f, unsigned off) {
  out32(0xcf8, 0x80000000u | b << 16 | d << 11 | f << 8 | (off & 252));
  return in32(0xcfc);
}
static void pci_write(unsigned b, unsigned d, unsigned f, unsigned off,
                      uint32_t value) {
  out32(0xcf8, 0x80000000u | b << 16 | d << 11 | f << 8 | (off & 252));
  out32(0xcfc, value);
}
#if OSLAB_PVH
// Optional pc-i440fx PVH board: root-bus MMIO aperture below chipset/APIC
// space. Firmware-assigned resources are never moved. Other memory BARs/bridges
// cause explicit rejection rather than guessing their occupied ranges.
static bool pvh_irq(unsigned dev, unsigned fn) {
  unsigned pin = pci_read(0, dev, fn, 0x3c) >> 8 & 255;
  if (!pin || pin > 4 || !(0xdef8u & (1u << nic_irq_line)) ||
      pci_read(0, 0, 0, 0) != 0x12378086)
    return false;
  for (unsigned d = 0; d < 32; d++)
    if (pci_read(0, d, 0, 0) == 0x70008086) {
      // Declared i440fx slot wiring: INTA..D swizzle into PIIX3 PIRQ A..D.
      unsigned shift = ((dev + pin - 2) & 3) * 8;
      uint32_t routes = pci_read(0, d, 0, 0x60);
      routes = (routes & ~(255u << shift)) | nic_irq_line << shift;
      pci_write(0, d, 0, 0x60, routes);
      unsigned port = 0x4d0 + nic_irq_line / 8, bit = 1u << (nic_irq_line % 8);
      out8(port,
           in8(port) | bit); // PCI INTx requires level-triggered PIC input.
      return (pci_read(0, d, 0, 0x60) >> shift & 255) == nic_irq_line &&
             (in8(port) & bit);
    }
  return false;
}
static bool pvh_bar(unsigned dev, unsigned fn, uint32_t flags,
                    uint32_t *address) {
  for (unsigned d = 0; d < 32; d++) {
    if (pci_read(0, d, 0, 0) == 0xffffffff)
      continue;
    unsigned functions = pci_read(0, d, 0, 12) & 0x800000 ? 8 : 1;
    for (unsigned f = 0; f < functions; f++) {
      if (pci_read(0, d, f, 0) == 0xffffffff)
        continue;
      if ((pci_read(0, d, f, 12) >> 16 & 127) != 0)
        return false;
      for (unsigned off = 16; off < 40; off += 4) {
        uint32_t bar = pci_read(0, d, f, off);
        if (bar & 1)
          continue;
        if (bar & ~15u)
          return false;
        if ((bar >> 1 & 3) == 2) {
          if (off == 36 || pci_read(0, d, f, off + 4))
            return false;
          off += 4;
        } else if (bar >> 1 & 3)
          return false;
      }
    }
  }
  uint32_t command = pci_read(0, dev, fn, 4) & 65535;
  pci_write(0, dev, fn, 4, command & ~3u);
  pci_write(0, dev, fn, 16, 0xffffffff);
  uint32_t mask = pci_read(0, dev, fn, 16) & ~15u;
  pci_write(0, dev, fn, 16, flags);
  pci_write(0, dev, fn, 4, command);
  uint32_t size = ~mask + 1;
  if (!mask || size < 0x20000 || size > 0x200000 || (size & (size - 1)))
    return false;
  struct map_record {
    uint64_t base, size;
    uint32_t type, attributes;
  } PACKED;
  unsigned count = *(volatile uint16_t *)(uintptr_t)0x5000;
  const struct map_record *map = (const void *)(uintptr_t)0x5010;
  if (!count || count > 64)
    return false;
  uint32_t candidate = 0xc0000000;
  for (; candidate < 0xe0000000; candidate += size) {
    bool occupied = false;
    for (unsigned i = 0; i < count; i++) {
      uint64_t end = map[i].base + map[i].size;
      if (end < map[i].base)
        return false;
      if (map[i].base < (uint64_t)candidate + size && candidate < end) {
        occupied = true;
        break;
      }
    }
    if (!occupied)
      break;
  }
  if (candidate >= 0xe0000000)
    return false;
  pci_write(0, dev, fn, 4, command & ~3u);
  pci_write(0, dev, fn, 16, candidate | flags);
  bool accepted = (pci_read(0, dev, fn, 16) & ~15u) == candidate;
  if (!accepted)
    pci_write(0, dev, fn, 16, flags);
  pci_write(0, dev, fn, 4, command);
  if (accepted)
    *address = candidate;
  return accepted;
}
#endif
static uint32_t reg(unsigned r) { return mmio[r / 4]; }
static void write_reg(unsigned r, uint32_t value) {
  mmio[r / 4] = value;
  barrier();
  (void)reg(8);
}
bool nic_link(void) { return mmio && (reg(8) & 2); }
bool nic_pending(void) { return rx && (rx[rx_head].status & 1); }
#if OSLAB_WEB_ONLY
bool nic_recent(void) { return received && milliseconds() - last_receive < 2; }
#endif
void nic_interrupt(unsigned irq) {
  if (mmio && irq == nic_irq_line)
    (void)reg(0xc0); // ICR read acknowledges/deasserts this device's causes.
}
bool nic_init(void) {
  unsigned bus = 0, dev = 0, fn = 0;
  bool found = false;
  for (unsigned b = 0; b < 256 && !found; b++)
    for (unsigned d = 0; d < 32 && !found; d++) {
      uint32_t id = pci_read(b, d, 0, 0);
      if (id == 0xffffffff)
        continue;
      unsigned funcs = pci_read(b, d, 0, 12) & 0x800000 ? 8 : 1;
      for (unsigned f = 0; f < funcs; f++)
        if ((id = pci_read(b, d, f, 0)) == 0x100e8086 || id == 0x10d38086) {
          bus = b;
          dev = d;
          fn = f;
          found = true;
          break;
        }
    }
  if (!found)
    return false;
  nic_irq_line = pci_read(bus, dev, fn, 0x3c) & 255;
  if (nic_irq_line < 3 || nic_irq_line >= 16)
    return false;
  uint32_t bar = pci_read(bus, dev, fn, 16);
  unsigned type = (bar >> 1) & 3;
  // Accept 64-bit BARs only when the mapping fits our 32-bit physical map.
  if ((bar & 1) || (type != 0 && type != 2) ||
      (type == 2 && pci_read(bus, dev, fn, 20)))
    return false;
  uint32_t address = bar & ~15u;
#if OSLAB_PVH
  if (bus || !pvh_irq(dev, fn))
    return false;
  if (!address && (type != 0 || !pvh_bar(dev, fn, bar, &address)))
    return false;
#endif
  if (!address || address < 0x400000)
    return false;
  // Mark the identity-map huge page containing registers uncacheable (PCD/PWT).
  volatile uint64_t *pd = (void *)(uintptr_t)0x92000;
  pd[address >> 21] |= 0x18;
  __asm__ volatile("invlpg (%0)" ::"r"((uintptr_t)address) : "memory");
  mmio = (void *)(uintptr_t)address;
  uint32_t command = pci_read(bus, dev, fn, 4);
  pci_write(bus, dev, fn, 4, (command & 65535 & ~(1u << 10)) | 6u);
  write_reg(0xd8, 0xffffffff);
  write_reg(0x100, 0);
  write_reg(0x400, 0);
  write_reg(0, reg(0) | (1u << 26));
  uint64_t deadline = milliseconds() + 100;
  while (reg(0) & (1u << 26)) {
    if (milliseconds() >= deadline) {
      mmio = NULL;
      return false;
    }
    idle();
  }
  write_reg(0xd8, 0xffffffff);
  (void)reg(0xc0);
  write_reg(0, reg(0) |
                   (1u << 6)); // SLU: negotiate link, preserve device defaults.
  uint32_t low = reg(0x5400), high = reg(0x5404);
  for (unsigned i = 0; i < 4; i++)
    nic_mac[i] = low >> (i * 8);
  nic_mac[4] = high;
  nic_mac[5] = high >> 8;
  if (!(high & (1u << 31)) || (nic_mac[0] & 1)) {
    mmio = NULL;
    return false;
  }
  // Legacy rings require 128-byte alignment. RING is a power of two >= 8,
  // so each ring occupies a multiple of 128 bytes; two rings never overlap.
  rx = pages_alloc((2 * RING * 16 + 4095) / 4096);
  tx = rx ? (void *)((uint8_t *)(void *)rx + RING * 16) : NULL;
  rx_data = pages_alloc(RING * BUFFER / 4096);
  tx_data = pages_alloc((TX_BUFFERS * BUFFER + 4095) / 4096);
  if (!rx || !tx || !rx_data || !tx_data)
    panic("nic-dma-memory");
  for (unsigned i = 0; i < RING; i++) {
    rx[i].address = (uintptr_t)(rx_data + i * BUFFER);
    tx[i].address = (uintptr_t)(tx_data + (i % TX_BUFFERS) * BUFFER);
    tx[i].status = 1;
  }
  for (unsigned i = 0; i < TX_BUFFERS; i++)
    tx_owner[i] = i;
  for (unsigned i = 0; i < 128; i++)
    write_reg(0x5200 + i * 4, 0);
  write_reg(0x2800, (uintptr_t)rx);
  write_reg(0x2804, 0);
  write_reg(0x2808, RING * 16);
  write_reg(0x2810, 0);
  write_reg(0x2818, RING - 1);
  write_reg(0x3800, (uintptr_t)tx);
  write_reg(0x3804, 0);
  write_reg(0x3808, RING * 16);
  write_reg(0x3810, 0);
  write_reg(0x3818, 0);
  write_reg(0x410, 10 | 8 << 10 | 6 << 20); // TIPG for copper legacy MAC.
  write_reg(0x400, 2 | 8 | 15 << 4 |
                       64 << 12); // EN, PSP, collision threshold/distance.
  write_reg(
      0x100,
      2 | (1u << 15) |
          (1u << 26));  // EN, broadcast accept, strip CRC, 2048-byte buffers.
  write_reg(0x2820, 0); // RDTR: publish RX completions without packet delay.
  write_reg(0x282c, 0); // RADV: no additional absolute RX delay.
  write_reg(0xc4, 64);  // ITR: cap interrupts at 61,035/s (256 ns units).
  (void)reg(0xc0);
  if (!irq_enable(nic_irq_line))
    return false;
  write_reg(0xd0, (1u << 7) | (1u << 6) | (1u << 4) | (1u << 2));
#if !OSLAB_PRODUCTION
  puts_os("OSL1 NIC driver=e1000");
  field("pci_bus", bus);
  field("pci_device", dev);
  field("ring", RING);
  putc_os('\n');
#endif
  deadline = milliseconds() + 3000;
  while (!nic_link() && milliseconds() < deadline)
    idle();
  return nic_link();
}
bool nic_send(const uint8_t *data, size_t size) {
  if (!mmio || size > 1514 || size < 14)
    return false;
  volatile struct tx_desc *d = &tx[tx_tail];
  unsigned slot = tx_tail % TX_BUFFERS;
  if (!(d->status & 1) || !(tx[tx_owner[slot]].status & 1)) {
#if !OSLAB_PRODUCTION
    nic_stats.tx_full++;
#endif
    return false;
  }
  memcpy(tx_data + slot * BUFFER, data, size);
  tx_owner[slot] = tx_tail;
  d->length = size;
  d->cso = 0;
  d->css = 0;
  d->special = 0;
  d->status = 0;
  d->command = 1 | 2 | 8; // EOP, insert Ethernet FCS, report completion.
  __asm__ volatile("sfence" ::: "memory");
  tx_tail = (tx_tail + 1) % RING;
  write_reg(0x3818, tx_tail);
#if !OSLAB_PRODUCTION
  nic_stats.tx_packets++;
  nic_stats.tx_bytes += size;
#endif
  return true;
}
void nic_poll(void (*receive)(const uint8_t *, size_t)) {
  if (!mmio)
    return;
  unsigned returned = 0, tail = rx_head;
  for (unsigned batch = 0; batch < RING; batch++) {
    volatile struct rx_desc *d = &rx[rx_head];
    if (!(d->status & 1))
      break;
    barrier();
    size_t len = d->length;
    if (d->errors || !(d->status & 2) || len < 14 || len > 1514) {
#if !OSLAB_PRODUCTION
      nic_stats.rx_errors++;
#endif
    } else {
#if !OSLAB_PRODUCTION
      nic_stats.rx_packets++;
      nic_stats.rx_bytes += len;
#endif
      receive(rx_data + rx_head * BUFFER, len);
    }
    d->status = 0;
    barrier();
    tail = rx_head;
    returned++;
    rx_head = (rx_head + 1) % RING;
  }
  // Return a whole processed batch with one MMIO doorbell/flush.
  if (returned) {
#if OSLAB_WEB_ONLY
    last_receive = milliseconds();
    received = true;
#endif
    write_reg(0x2818, tail);
  }
}

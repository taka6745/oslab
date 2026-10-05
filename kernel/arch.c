#include "net.h"
#include "os.h"
#include "x86.h"
struct idt_entry {
  uint16_t low, selector;
  uint8_t ist, type;
  uint16_t middle;
  uint32_t high, zero;
} PACKED;
struct idtr {
  uint16_t limit;
  uint64_t address;
} PACKED;
struct interrupt_frame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, rdx, rcx, rbx,
      rax, vector, error, rip, cs, flags, rsp, ss;
};
extern void *isr_table[48];
static struct idt_entry idt[256] __attribute__((aligned(16)));
volatile uint64_t timer_ticks;
static volatile uint64_t *hpet;
static uint64_t hpet_period;
uint64_t milliseconds(void) { return timer_ticks; }
#if OSLAB_PROFILE
static uint64_t perf_start, halt_ticks, halt_calls, interrupts;
void perf_reset(void) {
  __asm__ volatile("cli" ::: "memory");
  halt_ticks = halt_calls = interrupts = 0;
  perf_start = cycles();
  __asm__ volatile("sti" ::: "memory");
}
void perf_report(void) {
  __asm__ volatile("cli" ::: "memory");
  uint64_t elapsed = cycles() - perf_start;
  uint64_t halted = halt_ticks, calls = halt_calls, irqs = interrupts;
  __asm__ volatile("sti" ::: "memory");
  puts_os("OSL1 PERF clock=tsc scope=halt-through-interrupt-return");
  field("elapsed_ticks", elapsed);
  field("halt_ticks", halted);
  field("halt_calls", calls);
  field("interrupts", irqs);
  putc_os('\n');
}
#endif
void idle(void) {
#if OSLAB_WEB_ONLY
  // Poll only after recent RX activity; bound work before atomic sleep.
  if (nic_recent())
    for (unsigned i = 0; i < 256; i++) {
      if (nic_pending()) {
        __asm__ volatile("sti" ::: "memory");
        return;
      }
      __asm__ volatile("pause" ::: "memory");
    }
#endif
  // Check DMA with interrupts masked, then atomically enable-and-halt. A
  // packet arriving between polling and sleep must not wait for the PIT.
  __asm__ volatile("cli" ::: "memory");
  if (nic_pending()) {
    __asm__ volatile("sti" ::: "memory");
    return;
  }
#if OSLAB_PROFILE
  uint64_t start = cycles();
#endif
  __asm__ volatile("sti; hlt" ::: "memory");
#if OSLAB_PROFILE
  halt_ticks += cycles() - start;
  halt_calls++;
#endif
}
void interrupt_dispatch(struct interrupt_frame *f) {
  if (f->vector < 32) {
    __asm__ volatile("cli");
#if !OSLAB_PRODUCTION
    puts_os("OSL1 PANIC");
    field("vector", f->vector);
    field("error", f->error);
    puts_os(" rip=0x");
    number(f->rip, 16);
    puts_os(" rsp=0x");
    number(f->rsp, 16);
    uint64_t cr2;
    __asm__ volatile("mov %%cr2,%0" : "=r"(cr2));
    puts_os(" cr2=0x");
    number(cr2, 16);
    putc_os('\n');
#endif
    for (;;)
      __asm__ volatile("hlt");
  }
#if OSLAB_PROFILE
  interrupts++;
#endif
  if (f->vector == 32)
    timer_ticks = hpet ? clock_milliseconds(hpet[0xf0 / 8], hpet_period)
                       : timer_ticks + 1;
  else
    nic_interrupt(f->vector - 32);
  if (f->vector >= 40)
    out8(0xa0, 0x20);
  out8(0x20, 0x20);
}
bool irq_enable(unsigned irq) {
  if (irq < 3 || irq >= 16)
    return false;
  uint64_t flags;
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");
  if (irq >= 8) {
    out8(0xa1, in8(0xa1) & ~(1u << (irq - 8)));
    out8(0x21, in8(0x21) & ~(1u << 2));
  } else
    out8(0x21, in8(0x21) & ~(1u << irq));
  if (flags & (1u << 9))
    __asm__ volatile("sti" ::: "memory");
  return true;
}
void arch_init(void) {
#if OSLAB_PVH
  // This single-CPU PIC board does not depend on firmware's LAPIC virtual-wire
  // initialization. Disable LAPIC/x2APIC before relying on legacy PIC delivery.
  uint32_t a, b, c, d;
  __asm__ volatile("cpuid"
                   : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                   : "a"(1), "c"(0));
  if (d & (1u << 9)) {
    __asm__ volatile("rdmsr" : "=a"(a), "=d"(d) : "c"(0x1b));
    a &= ~((1u << 11) | (1u << 10));
    __asm__ volatile("wrmsr" ::"a"(a), "d"(d), "c"(0x1b) : "memory");
  }
#endif
  for (unsigned i = 0; i < 48; i++) {
    uintptr_t p = (uintptr_t)isr_table[i];
    idt[i] = (struct idt_entry){p, 0x18, 0, 0x8e, p >> 16, p >> 32, 0};
  }
  struct idtr r = {sizeof(idt) - 1, (uintptr_t)idt};
  __asm__ volatile("lidt %0" ::"m"(r));
  out8(0x20, 0x11);
  out8(0xa0, 0x11);
  out8(0x21, 32);
  out8(0xa1, 40);
  out8(0x21, 4);
  out8(0xa1, 2);
  out8(0x21, 1);
  out8(0xa1, 1);
  out8(0x21, 0xfe);
  out8(0xa1, 0xff); // NIC unmasks its firmware-assigned IRQ after DMA setup.
  // Supported PC board HPET location. Absence/32-bit counters fall back to PIT.
  // Cache elapsed counter time at each PIT IRQ: deferred IRQs cannot lose time.
  uintptr_t base = 0xfed00000;
  volatile uint64_t *pd = (void *)(uintptr_t)0x92000;
  pd[base >> 21] |= 0x18;
  __asm__ volatile("invlpg (%0)" ::"r"(base) : "memory");
  volatile uint64_t *clock = (void *)base;
  uint64_t capabilities = clock[0], period = capabilities >> 32;
  if ((capabilities & 0xff) && (capabilities & (1u << 13)) && period &&
      period <= 100000000) {
    clock[0x10 / 8] &=
        ~3ull; // retain PIT routing, disable HPET legacy replacement
    clock[0xf0 / 8] = 0;
    clock[0x10 / 8] |= 1;
    hpet_period = period;
    hpet = clock;
  }
  // PIT channel 0, mode 2, ~1000 Hz. No firmware timer service after boot.
  out8(0x43, 0x34);
  out8(0x40, 1193 & 255);
  out8(0x40, 1193 >> 8);
#if OSLAB_PROFILE
  perf_reset();
#else
  __asm__ volatile("sti");
#endif
}

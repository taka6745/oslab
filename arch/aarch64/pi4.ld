OUTPUT_FORMAT(elf64-littleaarch64)
ENTRY(pi_entry)
SECTIONS {
 . = 0x80000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) }
 .rodata ALIGN(16) : { *(.rodata .rodata.*) }
 .data ALIGN(16) : { *(.data .data.*) }
 .bss ALIGN(16) (NOLOAD) : { __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(8); __bss_end = .; }
 ASSERT(pi_entry == 0x80000, "Pi firmware load address mismatch")
 ASSERT(SIZEOF(.text) <= 49152, "Pi 4 L1 instruction budget exceeded")
 ASSERT(SIZEOF(.bss) + SIZEOF(.data) + SIZEOF(.rodata) <= 32768, "Pi 4 L1 data capacity budget exceeded")
 /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}

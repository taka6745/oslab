OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kernel_entry)
SECTION_ALIGN = DEFINED(OSLAB_SECTION_ALIGN) ? OSLAB_SECTION_ALIGN : 64;
SECTIONS {
 . = 0x100000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) }
 .rodata ALIGN(SECTION_ALIGN) : { *(.rodata .rodata.*) }
 .data ALIGN(SECTION_ALIGN) : { *(.data .data.*) }
 __bss_start = .;
 .bss ALIGN(SECTION_ALIGN) (NOLOAD) : { *(.bss .bss.*) *(COMMON) }
 __bss_end = .;
 __kernel_end = .;
 ASSERT(kernel_entry == 0x100000, "boot entry/load mismatch")
 ASSERT(__kernel_end < 0x400000, "kernel overlaps physical allocator")
 /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}

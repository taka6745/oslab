OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kernel_entry)
SECTIONS {
 . = 0x100000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) }
 .rodata ALIGN(4096) : { *(.rodata .rodata.*) }
 .data ALIGN(4096) : { *(.data .data.*) }
 __bss_start = .;
 .bss ALIGN(4096) (NOLOAD) : { *(.bss .bss.*) *(COMMON) }
 __bss_end = .;
 __kernel_end = .;
 ASSERT(kernel_entry == 0x100000, "boot entry/load mismatch")
 ASSERT(__kernel_end < 0x400000, "kernel overlaps physical allocator")
 /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}

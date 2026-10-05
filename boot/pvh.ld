OUTPUT_FORMAT(elf32-i386)
ENTRY(pvh_entry)
PHDRS { text PT_LOAD FLAGS(5); note PT_NOTE FLAGS(4); }
SECTIONS {
 . = 0x200000;
 .note.Xen : { KEEP(*(.note.Xen)) } :text :note
 .text ALIGN(16) : { *(.text) } :text
 .rodata ALIGN(16) : { *(.rodata) } :text
 ASSERT(. < 0x400000, "PVH adapter overlaps allocator")
 /DISCARD/ : { *(.note.GNU-stack) }
}

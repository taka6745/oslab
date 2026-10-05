OUTPUT_FORMAT(elf32-i386)
ENTRY(stage2)
SECTIONS { . = 0x8000; .text : { *(.text) } /DISCARD/ : { *(.eh_frame) } }

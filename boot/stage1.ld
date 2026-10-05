OUTPUT_FORMAT(elf32-i386)
ENTRY(stage1)
SECTIONS { . = 0x7c00; .text : { *(.text) } /DISCARD/ : { *(.eh_frame) } }

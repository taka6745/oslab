# Guest-only build definitions. Output defaults to the external harness checkout.
OUT ?= ../osenv/build/oslab
CC := /opt/homebrew/opt/llvm/bin/clang
LD := /opt/homebrew/opt/lld/bin/ld.lld
OBJCOPY := /opt/homebrew/opt/llvm/bin/llvm-objcopy
NASM := /opt/homebrew/opt/nasm/bin/nasm
DEBUG ?= 1
PYTHON ?= python3.12
CFLAGS := --target=x86_64-unknown-none-elf -std=c11 -O2 -g -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -mgeneral-regs-only -Wall -Wextra -Werror -Iinclude -DOSLAB_DEBUG=$(DEBUG)
CSRC := $(wildcard kernel/*.c)
OBJ := $(patsubst kernel/%.c,$(OUT)/%.o,$(CSRC)) $(OUT)/entry.o $(OUT)/interrupts.o
.PHONY: all clean host-test
all: $(OUT)/oslab.img
$(OUT):
	mkdir -p $@
$(OUT)/%.o: kernel/%.c $(wildcard include/*.h) | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@
$(OUT)/entry.o: arch/x86_64/entry.asm | $(OUT)
	$(NASM) -f elf64 -g -F dwarf $< -o $@
$(OUT)/interrupts.o: arch/x86_64/interrupts.asm | $(OUT)
	$(NASM) -f elf64 -g -F dwarf $< -o $@
$(OUT)/kernel.elf: $(OBJ) linker.ld
	$(LD) -m elf_x86_64 -nostdlib -T linker.ld $(OBJ) -o $@
$(OUT)/kernel.bin: $(OUT)/kernel.elf
	$(OBJCOPY) -O binary $< $@
# FNV-1a is an accidental-corruption check, not a cryptographic authenticator.
$(OUT)/stage2.o: boot/stage2.asm boot/serial.inc $(OUT)/kernel.bin
	$(NASM) -f elf32 -g -F dwarf -D "KERNEL_BYTES=$$(wc -c < $(OUT)/kernel.bin)" -D KERNEL_SECTORS=$$((($$(wc -c < $(OUT)/kernel.bin)+511)/512)) -D KERNEL_HASH=$$($(PYTHON) -c 'from functools import reduce; print(reduce(lambda h,b:((h^b)*16777619)& 0xffffffff,open("$(OUT)/kernel.bin","rb").read(),2166136261))') $< -o $@
$(OUT)/stage2.elf: $(OUT)/stage2.o boot/stage2.ld
	$(LD) -m elf_i386 -T boot/stage2.ld $< -o $@
$(OUT)/stage2.bin: $(OUT)/stage2.elf
	$(OBJCOPY) -O binary $< $@
$(OUT)/stage1.o: boot/stage1.asm boot/serial.inc $(OUT)/stage2.bin
	$(NASM) -f elf32 -g -F dwarf -D STAGE2_HASH=$$($(PYTHON) -c 'from functools import reduce; print(reduce(lambda h,b:((h^b)*16777619)& 0xffffffff,open("$(OUT)/stage2.bin","rb").read(),2166136261))') $< -o $@
$(OUT)/stage1.elf: $(OUT)/stage1.o boot/stage1.ld
	$(LD) -m elf_i386 -T boot/stage1.ld $< -o $@
$(OUT)/stage1.bin: $(OUT)/stage1.elf
	$(OBJCOPY) -O binary $< $@
$(OUT)/oslab.img: $(OUT)/stage1.bin $(OUT)/stage2.bin $(OUT)/kernel.bin
	$(PYTHON) -c 'from pathlib import Path; p=Path("$(OUT)"); a=(p/"stage1.bin").read_bytes(); b=(p/"stage2.bin").read_bytes(); c=(p/"kernel.bin").read_bytes(); assert len(a)==512 and a[-2:]==bytes([85,170]) and len(b)==4096 and 0<len(c)<=524288; image=a+b+c; (p/"oslab.img").write_bytes(image+bytes((-len(image))%512))'
clean:
	rm -f $(OUT)/*.o $(OUT)/*.elf $(OUT)/*.bin $(OUT)/oslab.img
host-test: | $(OUT)
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Iinclude kernel/packets.c tests/packets.c -o $(OUT)/packets-test
	$(OUT)/packets-test

# Guest-only build definitions. Output defaults to the external harness checkout.
OUT ?= ../osenv/build/oslab
CC := /opt/homebrew/opt/llvm/bin/clang
LD := /opt/homebrew/opt/lld/bin/ld.lld
OBJCOPY := /opt/homebrew/opt/llvm/bin/llvm-objcopy
NASM := /opt/homebrew/opt/nasm/bin/nasm
DEBUG ?= 1
PRODUCTION ?= 0
PROFILE ?= 0
LTO ?= 0
STAGE2_BYTES := 1024
BOOT_COMPRESS ?= $(PRODUCTION)
ifneq ($(BOOT_COMPRESS),0)
ifneq ($(BOOT_COMPRESS),1)
$(error BOOT_COMPRESS must be 0 or 1)
endif
endif
OPT ?= 2
AUTOSERVE ?= 0
WEB_ONLY ?= 0
RAM_LIMIT ?= 0x100000000ull
NIC_RING ?= 64
TX_BUFFERS ?= $(NIC_RING)
STACK_BYTES ?= 32768
PYTHON ?= python3.12
override CFLAGS := --target=x86_64-unknown-none-elf -std=c11 -O$(OPT) -g -ffreestanding -nostdinc -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -mgeneral-regs-only -Wall -Wextra -Werror -Iinclude -DOSLAB_PRODUCTION=$(PRODUCTION) -DOSLAB_PROFILE=$(PROFILE) -DOSLAB_DEBUG=$(DEBUG) -DOSLAB_AUTOSERVE=$(AUTOSERVE) -DOSLAB_RAM_LIMIT=$(RAM_LIMIT) -DOSLAB_NIC_RING=$(NIC_RING) -DOSLAB_WEB_ONLY=$(WEB_ONLY) -DOSLAB_TX_BUFFERS=$(TX_BUFFERS)
ifeq ($(LTO),1)
override CFLAGS += -flto
endif
ifeq ($(WEB_ONLY),1)
override CFLAGS += -ffunction-sections -fdata-sections
GUEST_GC := --gc-sections --defsym=OSLAB_SECTION_ALIGN=64
endif
CSRC := $(wildcard kernel/*.c)
OBJ := $(patsubst kernel/%.c,$(OUT)/%.o,$(CSRC)) $(OUT)/entry.o $(OUT)/interrupts.o
.PHONY: all clean host-test check-inputs FORCE
all: $(OUT)/oslab.img check-inputs
$(OUT):
	mkdir -p $@
FORCE:
$(OUT)/build-config: FORCE | $(OUT)
	$(PYTHON) -c 'from pathlib import Path; p=Path("$@"); value="$(CFLAGS) STACK_BYTES=$(STACK_BYTES) STAGE2_BYTES=$(STAGE2_BYTES) BOOT_COMPRESS=$(BOOT_COMPRESS) GC=$(GUEST_GC)"; p.write_text(value) if not p.exists() or p.read_text()!=value else None'
$(OUT)/%.o: kernel/%.c $(wildcard include/*.h) Makefile $(OUT)/build-config | $(OUT)
	$(CC) $(CFLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/entry.o: arch/x86_64/entry.asm Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -f elf64 -g -F dwarf -MD $@.d -DSTACK_BYTES=$(STACK_BYTES) $< -o $@
$(OUT)/interrupts.o: arch/x86_64/interrupts.asm Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -f elf64 -g -F dwarf -MD $@.d -DSTACK_BYTES=$(STACK_BYTES) $< -o $@
$(OUT)/kernel.elf: $(OBJ) linker.ld
	$(LD) -m elf_x86_64 -nostdlib $(GUEST_GC) -T linker.ld $(OBJ) -o $@
$(OUT)/kernel.bin: $(OUT)/kernel.elf
	$(OBJCOPY) -O binary $< $@
# Host-only transform; libc is not linked into the guest.
$(OUT)/pack-tool: boot/pack.c Makefile | $(OUT)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
ifeq ($(BOOT_COMPRESS),1)
$(OUT)/kernel.payload: $(OUT)/kernel.bin $(OUT)/pack-tool $(OUT)/build-config
	$(OUT)/pack-tool < $< > $@
	$(PYTHON) -c 'from pathlib import Path; p=Path("$(OUT)"); assert 0<(p/"kernel.payload").stat().st_size<(p/"kernel.bin").stat().st_size, "compressed kernel must actually shrink"'
else
$(OUT)/kernel.payload: $(OUT)/kernel.bin $(OUT)/build-config
	cp $< $@
endif
# FNV-1a is an accidental-corruption check, not a cryptographic authenticator.
$(OUT)/stage2.o: boot/stage2.asm boot/serial.inc Makefile $(OUT)/build-config $(OUT)/kernel.bin $(OUT)/kernel.payload
	$(NASM) -DOSLAB_PRODUCTION=$(PRODUCTION) -f elf32 -g -F dwarf -MD $@.d -D KERNEL_COMPRESSED=$(BOOT_COMPRESS) -D "KERNEL_STORAGE_BYTES=$$(wc -c < $(OUT)/kernel.payload)" -D KERNEL_LBA=$$((1+$(STAGE2_BYTES)/512)) -D STAGE2_BYTES=$(STAGE2_BYTES) -D "KERNEL_BYTES=$$(wc -c < $(OUT)/kernel.bin)" -D KERNEL_SECTORS=$$((($$(wc -c < $(OUT)/kernel.payload)+511)/512)) -D KERNEL_HASH=$$($(PYTHON) -c 'from functools import reduce; print(reduce(lambda h,b:((h^b)*16777619)& 0xffffffff,open("$(OUT)/kernel.bin","rb").read(),2166136261))') $< -o $@
$(OUT)/stage2.elf: $(OUT)/stage2.o boot/stage2.ld
	$(LD) -m elf_i386 -T boot/stage2.ld $< -o $@
$(OUT)/stage2.bin: $(OUT)/stage2.elf
	$(OBJCOPY) -O binary $< $@
$(OUT)/stage1.o: boot/stage1.asm boot/serial.inc Makefile $(OUT)/build-config $(OUT)/stage2.bin
	$(NASM) -DOSLAB_PRODUCTION=$(PRODUCTION) -f elf32 -g -F dwarf -MD $@.d -D STAGE2_BYTES=$(STAGE2_BYTES) -D STAGE2_HASH=$$($(PYTHON) -c 'from functools import reduce; print(reduce(lambda h,b:((h^b)*16777619)& 0xffffffff,open("$(OUT)/stage2.bin","rb").read(),2166136261))') $< -o $@
$(OUT)/stage1.elf: $(OUT)/stage1.o boot/stage1.ld
	$(LD) -m elf_i386 -T boot/stage1.ld $< -o $@
$(OUT)/stage1.bin: $(OUT)/stage1.elf
	$(OBJCOPY) -O binary $< $@
$(OUT)/oslab.img: $(OUT)/stage1.bin $(OUT)/stage2.bin $(OUT)/kernel.payload
	$(PYTHON) -c 'from pathlib import Path; p=Path("$(OUT)"); a=(p/"stage1.bin").read_bytes(); b=(p/"stage2.bin").read_bytes(); c=(p/"kernel.payload").read_bytes(); assert len(a)==512 and a[-2:]==bytes([85,170]) and len(b)==$(STAGE2_BYTES) and 0<len(c)<=524288; image=a+b+c; (p/"oslab.img").write_bytes(image+bytes((-len(image))%512))'
clean:
	rm -f $(OUT)/*.o $(OUT)/*.elf $(OUT)/*.bin $(OUT)/oslab.img
RUNTIME_HOST_FLAGS ?= $(if $(filter Darwin,$(shell uname -s)),-arch x86_64,)
host-test: | $(OUT)
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Iinclude kernel/packets.c tests/packets.c -o $(OUT)/packets-test
	$(OUT)/packets-test
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude kernel/http.c tests/http.c -o $(OUT)/http-test
	$(OUT)/http-test
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -Dmemcpy=os_memcpy -Dmemset=os_memset -Dmemcmp=os_memcmp -Dstrlen=os_strlen -c kernel/runtime.c -o $(OUT)/runtime-host.o
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) tests/runtime.c $(OUT)/runtime-host.o -o $(OUT)/runtime-test
	$(OUT)/runtime-test

# Reject resolved compiler/assembler inputs outside this source repository.
# Host-test dependencies are deliberately separate and never feed the guest link.
check-inputs: $(OUT)/kernel.elf $(OUT)/stage1.elf $(OUT)/stage2.elf
	$(PYTHON) -c 'from pathlib import Path; import shlex,json,hashlib; root=Path.cwd().resolve(); files={Path(name).resolve() for d in [Path(n+".d") for n in "$(OBJ) $(OUT)/stage1.o $(OUT)/stage2.o".split()] for name in shlex.split(d.read_text().replace("\\\n"," ").split(":",1)[1])}; files.update(Path(n).resolve() for n in ("Makefile","linker.ld","boot/stage1.ld","boot/stage2.ld","boot/pack.c")); assert files, "missing guest dependency evidence"; outside=[str(p) for p in files if not p.is_relative_to(root)]; assert not outside, "external guest source inputs: "+repr(outside); result={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}; Path("$(OUT)/source-inputs.json").write_text(json.dumps(result,indent=2)+"\n"); print("Guest input boundary: "+str(len(files))+" repository-owned inputs")'

# Separate, real Pi 4 CPU/UART bring-up; Ethernet is not implemented for this board.
# Use a separate OUT. This target does not substitute for the BIOS acceptance gate.
.PHONY: pi4-bringup
PI_FLAGS := --target=aarch64-unknown-none-elf -std=c11 -Os -g -ffreestanding -nostdinc -fno-builtin -fno-stack-protector -mgeneral-regs-only -Wall -Wextra -Werror -Iinclude
PI_OBJ := $(OUT)/pi-entry.o $(OUT)/pi-board.o $(OUT)/pi-runtime.o $(OUT)/pi-http.o $(OUT)/pi-format.o
$(OUT)/pi-entry.o: arch/aarch64/entry.S Makefile | $(OUT)
	$(CC) --target=aarch64-unknown-none-elf -nostdinc -MD -MF $@.d -c $< -o $@
$(OUT)/pi-board.o: arch/aarch64/pi4.c $(wildcard include/*.h) Makefile | $(OUT)
	$(CC) $(PI_FLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/pi-runtime.o: kernel/runtime.c $(wildcard include/*.h) Makefile | $(OUT)
	$(CC) $(PI_FLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/pi-format.o: kernel/format.c $(wildcard include/*.h) Makefile | $(OUT)
	$(CC) $(PI_FLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/pi-http.o: kernel/http.c $(wildcard include/*.h) Makefile | $(OUT)
	$(CC) $(PI_FLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/pi4.elf: $(PI_OBJ) arch/aarch64/pi4.ld
	$(LD) -m aarch64elf -nostdlib -T arch/aarch64/pi4.ld $(PI_OBJ) -o $@
$(OUT)/kernel8.img: $(OUT)/pi4.elf
	$(OBJCOPY) -O binary $< $@
pi4-bringup: $(OUT)/kernel8.img
	$(PYTHON) -c 'from pathlib import Path; import shlex; root=Path.cwd().resolve(); files={Path(n).resolve() for o in "$(PI_OBJ)".split() for n in shlex.split(Path(o+".d").read_text().replace(chr(92)+chr(10)," ").split(":",1)[1])}; assert all(p.is_relative_to(root) for p in files), "external Pi source input"; print("Pi guest inputs repository-owned")'

.PHONY: web prod debug web-debug
WEB_LTO ?= 1
WEB_OPT ?= z
WEB_OUT ?= ../osenv/build/oslab-web
PROD_OUT ?= ../osenv/build/oslab-prod
DEBUG_OUT ?= ../osenv/build/oslab-debug
web: prod
prod:
	$(MAKE) all OPT=$(WEB_OPT) LTO=$(WEB_LTO) PRODUCTION=1 DEBUG=0 PROFILE=0 AUTOSERVE=1 WEB_ONLY=1 RAM_LIMIT=0x4000000ull NIC_RING=8 TX_BUFFERS=2 STACK_BYTES=16384 OUT=$(PROD_OUT)
debug:
	$(MAKE) all PRODUCTION=0 DEBUG=1 PROFILE=1 AUTOSERVE=0 WEB_ONLY=0 OUT=$(DEBUG_OUT)
web-debug:
	$(MAKE) all OPT=$(WEB_OPT) LTO=$(WEB_LTO) PRODUCTION=0 DEBUG=1 PROFILE=1 AUTOSERVE=1 WEB_ONLY=1 RAM_LIMIT=0x4000000ull NIC_RING=8 TX_BUFFERS=2 STACK_BYTES=16384 OUT=$(WEB_OUT)

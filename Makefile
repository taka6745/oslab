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
MACHINE ?= 0
MACHINE_HTTP ?= 0
MACHINE_HEADERS ?= $(MACHINE_HTTP)
ifneq ($(MACHINE_HEADERS),0)
ifneq ($(MACHINE_HEADERS),1)
$(error MACHINE_HEADERS must be 0 or 1)
endif
ifeq ($(MACHINE_HTTP),0)
$(error MACHINE_HEADERS requires MACHINE_HTTP=1)
endif
endif
ifneq ($(MACHINE_HTTP),0)
ifneq ($(MACHINE_HTTP),1)
$(error MACHINE_HTTP must be 0 or 1)
endif
endif
ifneq ($(MACHINE),0)
ifneq ($(MACHINE),1)
$(error MACHINE must be 0 or 1)
endif
endif
PYTHON ?= python3.12
override CFLAGS := --target=x86_64-unknown-none-elf -std=c11 -O$(OPT) -g -ffreestanding -nostdinc -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -mgeneral-regs-only -Wall -Wextra -Werror -Iinclude -DOSLAB_PRODUCTION=$(PRODUCTION) -DOSLAB_PROFILE=$(PROFILE) -DOSLAB_DEBUG=$(DEBUG) -DOSLAB_AUTOSERVE=$(AUTOSERVE) -DOSLAB_RAM_LIMIT=$(RAM_LIMIT) -DOSLAB_NIC_RING=$(NIC_RING) -DOSLAB_WEB_ONLY=$(WEB_ONLY) -DOSLAB_TX_BUFFERS=$(TX_BUFFERS)
ifeq ($(LTO),1)
override CFLAGS += -flto
endif
ifeq ($(WEB_ONLY),1)
override CFLAGS += -ffunction-sections -fdata-sections
GUEST_GC := --gc-sections --defsym=OSLAB_SECTION_ALIGN=64
endif
override CFLAGS += -DOSLAB_MACHINE=$(MACHINE) -DOSLAB_MACHINE_HTTP=$(MACHINE_HTTP) -DOSLAB_MACHINE_HEADERS=$(MACHINE_HEADERS)
CSRC := $(wildcard kernel/*.c)
OBJ := $(patsubst kernel/%.c,$(OUT)/%.o,$(CSRC)) $(OUT)/entry.o $(OUT)/interrupts.o
ENTRY_SOURCE := arch/x86_64/entry.asm
IRQ_SOURCE := arch/x86_64/interrupts.asm
PVH_SOURCE := boot/pvh.asm
ifeq ($(MACHINE),1)
ENTRY_SOURCE := kernel/machine/entry.asm
IRQ_SOURCE := kernel/machine/interrupts.asm
PVH_SOURCE := boot/machine/pvh.asm
OBJ += $(OUT)/machine-hotpath.o
endif
ifeq ($(MACHINE_HTTP),1)
OBJ += $(OUT)/machine-http.o
endif
ifeq ($(MACHINE_HEADERS),1)
OBJ += $(OUT)/machine-headers.o
endif
.PHONY: all clean host-test check-inputs FORCE
all: $(OUT)/oslab.img check-inputs
$(OUT):
	mkdir -p $@
FORCE:
$(OUT)/build-config: FORCE | $(OUT)
	$(PYTHON) -c 'from pathlib import Path; p=Path("$@"); value="$(CFLAGS) STACK_BYTES=$(STACK_BYTES) STAGE2_BYTES=$(STAGE2_BYTES) BOOT_COMPRESS=$(BOOT_COMPRESS) GC=$(GUEST_GC)"; p.write_text(value) if not p.exists() or p.read_text()!=value else None'
$(OUT)/%.o: kernel/%.c $(wildcard include/*.h) Makefile $(OUT)/build-config | $(OUT)
	$(CC) $(CFLAGS) -MD -MF $@.d -c $< -o $@
$(OUT)/entry.o: $(ENTRY_SOURCE) Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -w+error -f elf64 -g -F dwarf -MD $@.d -DSTACK_BYTES=$(STACK_BYTES) $< -o $@
$(OUT)/interrupts.o: $(IRQ_SOURCE) Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -w+error -f elf64 -g -F dwarf -MD $@.d -DSTACK_BYTES=$(STACK_BYTES) $< -o $@
$(OUT)/machine-hotpath.o: kernel/machine/hotpath.asm Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -w+error -f elf64 -g -F dwarf -MD $@.d -DGUEST_EXPORTS=1 $< -o $@
$(OUT)/machine-http.o: kernel/machine/http.asm Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -w+error -f elf64 -g -F dwarf -MD $@.d -DGUEST_EXPORTS=1 $< -o $@
$(OUT)/machine-headers.o: kernel/machine/headers.asm Makefile $(OUT)/build-config | $(OUT)
	$(NASM) -w+error -f elf64 -g -F dwarf -MD $@.d $< -o $@
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
	$(CC) -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -c kernel/clock.c -o $(OUT)/clock-host.o
	$(CC) -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) tests/clock.c $(OUT)/clock-host.o -o $(OUT)/clock-test
	$(OUT)/clock-test

HOST_ASM_FORMAT := $(if $(filter Darwin,$(shell uname -s)),macho64,elf64)
.PHONY: machine-host-test machine-prod machine-pvh-prod machine-debug
machine-host-test: | $(OUT)
	$(NASM) -f elf64 -DCASE=0 tests/machine/branch.asm -o $(OUT)/machine-branch.o
	@for case in 1 2 3; do if $(NASM) -f elf64 -DCASE=$$case tests/machine/branch.asm -o $(OUT)/machine-branch-bad.o >$(OUT)/branch-$$case.log 2>&1; then exit 1; fi; done
	$(NASM) -w+error -f $(HOST_ASM_FORMAT) kernel/machine/hotpath.asm -o $(OUT)/machine-host.o
	$(CC) -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) tests/machine/hotpath.c $(OUT)/machine-host.o -o $(OUT)/machine-host-test
	$(OUT)/machine-host-test
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -DOSLAB_MACHINE=1 -Dchecksum=machine_checksum -Dtransport_checksum=machine_transport_checksum kernel/packets.c tests/packets.c $(OUT)/machine-host.o -o $(OUT)/machine-packets-test
	$(OUT)/machine-packets-test
	$(NASM) -w+error -f $(HOST_ASM_FORMAT) kernel/machine/http.asm -o $(OUT)/machine-http-host.o
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -Dhttp_select=readable_http_select -c kernel/http.c -o $(OUT)/readable-http-host.o
	$(NASM) -w+error -f $(HOST_ASM_FORMAT) kernel/machine/headers.asm -o $(OUT)/machine-headers-host.o
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -DOSLAB_MACHINE_HTTP=1 -DOSLAB_MACHINE_HEADERS=1 -c kernel/http.c -o $(OUT)/machine-http-data-host.o
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude tests/machine/http.c $(OUT)/readable-http-host.o $(OUT)/machine-http-data-host.o $(OUT)/machine-http-host.o $(OUT)/machine-headers-host.o -o $(OUT)/machine-http-test
	$(OUT)/machine-http-test
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror $(RUNTIME_HOST_FLAGS) -Iinclude -Dhttp_select=machine_http_select tests/http.c $(OUT)/machine-http-data-host.o $(OUT)/machine-http-host.o $(OUT)/machine-headers-host.o -o $(OUT)/machine-http-regression-test
	$(OUT)/machine-http-regression-test
MACHINE_OUT ?= ../osenv/build/oslab-machine-prod
MACHINE_PVH_OUT ?= ../osenv/build/oslab-machine-pvh
MACHINE_DEBUG_OUT ?= ../osenv/build/oslab-machine-debug
machine-prod:
	$(MAKE) prod MACHINE=1 PROD_OUT=$(MACHINE_OUT)
machine-pvh-prod:
	$(MAKE) pvh-prod MACHINE=1 PVH_OUT=$(MACHINE_PVH_OUT)
machine-debug:
	$(MAKE) debug MACHINE=1 DEBUG_OUT=$(MACHINE_DEBUG_OUT)

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
WEB_OPT ?= 3
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

# Optional PVH32 entry, retaining all mandatory BIOS disk-boot artifacts/checks.
PVH ?= 0
PVH_PRELOAD ?= 0
ifneq ($(PVH_PRELOAD),0)
ifneq ($(PVH_PRELOAD),1)
$(error PVH_PRELOAD must be 0 or 1)
endif
endif
override CFLAGS += -DOSLAB_PVH=$(PVH)
# The embedded kernel.bin is generated solely from check-inputs' authored sources.
PVH_OUT ?= ../osenv/build/oslab-pvh-prod
PVH_DEBUG_OUT ?= ../osenv/build/oslab-pvh-debug
.PHONY: pvh pvh-prod pvh-debug pvh-preload-prod check-pvh-inputs
$(OUT)/pvh-config: FORCE | $(OUT)
	$(PYTHON) -c 'from pathlib import Path; p=Path("$@"); value="PVH_PRELOAD=$(PVH_PRELOAD)"; p.write_text(value) if not p.exists() or p.read_text()!=value else None'
$(OUT)/pvh.o: $(PVH_SOURCE) Makefile $(OUT)/build-config $(OUT)/pvh-config $(OUT)/kernel.bin
	$(NASM) -w+error -DOSLAB_PRODUCTION=$(PRODUCTION) -DKERNEL_PRELOADED=$(PVH_PRELOAD) -f elf32 -g -F dwarf -MD $@.d -D 'KERNEL_FILE="$(OUT)/kernel.bin"' -D "KERNEL_BYTES=$$(wc -c < $(OUT)/kernel.bin)" -D KERNEL_HASH=$$($(PYTHON) -c 'from functools import reduce; print(reduce(lambda h,b:((h^b)*16777619)&0xffffffff,open("$(OUT)/kernel.bin","rb").read(),2166136261))') $< -o $@
$(OUT)/pvh.elf: $(OUT)/pvh.o boot/pvh.ld
	$(LD) -m elf_i386 -nostdlib -T boot/pvh.ld $< -o $@
check-pvh-inputs: check-inputs $(OUT)/pvh.elf
	$(PYTHON) -c 'from pathlib import Path; import shlex,json,hashlib; root=Path.cwd().resolve(); out=Path("$(OUT)").resolve(); generated=(out/"kernel.bin").resolve(); inputs={Path(n).resolve() for n in shlex.split((out/"pvh.o.d").read_text().replace(chr(92)+chr(10)," ").split(":",1)[1])}; preload=bool($(PVH_PRELOAD)); assert preload or generated in inputs, "PVH missing generated kernel dependency"; source=inputs-{generated}; source.update((root/"boot/pvh.ld",root/"Makefile")); assert source and all(p.is_relative_to(root) for p in source), "external PVH guest source"; assert 0<generated.stat().st_size<=524288, "PVH kernel size"; result={"preload":preload,"sources":json.loads((out/"source-inputs.json").read_text()),"adapter_sources":{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source)},"generated_artifacts":{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (generated,out/"kernel.elf",out/"oslab.img",out/"pvh.elf")},"configuration":(out/"build-config").read_text()}; (out/"pvh-inputs.json").write_text(json.dumps(result,indent=2)+"\n"); print("PVH boundary: authored adapter plus generated, source-verified kernel")'
pvh: all $(OUT)/pvh.elf check-pvh-inputs
pvh-prod:
	$(MAKE) pvh PVH=1 OPT=$(WEB_OPT) LTO=$(WEB_LTO) PRODUCTION=1 DEBUG=0 PROFILE=0 AUTOSERVE=1 WEB_ONLY=1 RAM_LIMIT=0x4000000ull NIC_RING=8 TX_BUFFERS=2 STACK_BYTES=16384 OUT=$(PVH_OUT)
pvh-debug:
	$(MAKE) pvh PVH=1 PRODUCTION=0 DEBUG=1 PROFILE=1 AUTOSERVE=0 WEB_ONLY=0 OUT=$(PVH_DEBUG_OUT)

# RAM preload is a distinct reset-boot route; the host loads the recorded kernel.
PRELOAD_OUT ?= ../osenv/build/oslab-pvh-preload
pvh-preload-prod:
	$(MAKE) pvh PVH=1 PVH_PRELOAD=1 OPT=$(WEB_OPT) LTO=$(WEB_LTO) PRODUCTION=1 DEBUG=0 PROFILE=0 AUTOSERVE=1 WEB_ONLY=1 RAM_LIMIT=0x4000000ull NIC_RING=8 TX_BUFFERS=2 STACK_BYTES=16384 OUT=$(PRELOAD_OUT)

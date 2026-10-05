# Raw primitives and HTTP

Goal: complete raw-byte bounded memory/checksum/HTTP primitives. Interface:
SysV x86-64, general registers only, DF clear, no red zone; preserved RBX,
RBP and R12–R15. Actual placement and image hashes come from the external
build manifest; this module contains code and response data.
Prerequisites: readable mapped arguments, mapped destination capacity, zero DF.

- `checksum(RDI=data, RSI=size)` returns the network-order numeric checksum
  in EAX. Empty input returns 65535. Each 64-bit carry propagates through a bounded four-word chain and
  is folded before pointer/count arithmetic; 32-bit and 16-bit folds consume carry before subsequent arithmetic.
  Odd tails read one byte; multiword reads require sufficient remaining size.
- `transport_checksum(EDI=source, ESI=destination, EDX=protocol,
  RCX=data, R8=size)` uses numeric IPv4 addresses from `include/net.h` and
  pseudoheader length/protocol. Length over 65535 returns 1, without data reads.
- `mem_copy(RDI=destination, RSI=source, RDX=size)` returns original destination
  in RAX. Regions must not overlap. `mem_zero(RDI=destination, RSI=size)` clears
  exactly that many bytes and returns zero in RAX. Both routines process whole
  qwords followed by the remaining zero to seven bytes, including unaligned
  buffers. Neither operation accesses outside its specified region.
- `http_select(RDI=request, RSI=size)` returns zero while the header delimiter
  is absent; otherwise RAX points to two qwords `{data,size}`. More than 768
  bytes selects 400. The delimiter must end the supplied request. HTTP/1.0 and
  HTTP/1.1 accept GET or HEAD with a slash-prefixed target; `/` selects 200,
  another target 404, other methods 405. Invalid syntax selects 400. HEAD omits
  the body while retaining the GET Content-Length. Requests require printable
  request-line ASCII, valid header-name tokens and bounded ASCII/tab values;
  HTTP/1.1 requires one nonempty Host without internal whitespace. Duplicate
  Host or Content-Length, transfer encoding and nonzero request bodies fail.

Response descriptors: invalid, method, missing, page, HEAD missing, HEAD page.
The page response is exactly 1460 bytes, including its original 1366-byte page;
400/405/404 wire sizes are 79/105/75. HEAD missing/page sizes are 65/94.

Provenance: explicit instructions were transcribed from this project's authored
`kernel/machine/hotpath.asm`, `http.asm` and `headers.asm`. A host Python standard
library text transformation converted their literal db/dw/dd bytes and symbolic
branches to raw grammar; it never assembled instructions, imported binary bytes,
or invoked a compiler/linker. New instructions adapt the return ABI, implement
zero fill, and recognize HEAD. Independently checked against x86 encoding fields:
REX.W memory/64-bit operations; SIB base+index addressing; Jcc short/near forms;
RIP-relative LEA/call relocation bases; immediate widths; preserved-register push
and reverse pop order; five selector pushes align its validator call. Header
validator's nested name calls use no stack-alignment-sensitive operations.
Literal HTTP/page data came from our `kernel/http.c` string constants via Python
literal decoding. The 405 Allow field now truthfully lists GET and HEAD. No
third-party guest code or executable-byte extraction was used.

Acceptance: symbolic fields and signed branch bounds resolve; independent
instruction disassembly checks the authored encodings and destinations. All six
wire descriptors were independently decoded and their framing/lengths checked.
Actual BIOS-booted reciprocal image passed 186 primitive checks: seeded lengths,
alignments/carry, transport bounds, copy/zero guard pages, HTTP syntax/HEAD and
767/768/769-byte boundaries. A loaded checksum instruction mutant was rejected
and restored. A further 286 clock/guard checks cover 251 real HPET counter
vectors, a rejected/restored reciprocal instruction mutant, all zero-fill tails
0–31 at an unmapped boundary, and real HPET progression. Tests restore state,
retain external evidence and stop their VMs; counter-injection continuity is not
claimed. Evidence: `osenv/local/t021/reciprocal-gate/report.json`. These are local
QEMU checks; homelab acceptance is recorded separately for the exact image.


T023 replaces eight accumulation instructions per32-byte block with five:
ADD, three ADC memory operands, final ADC0. Bounds, loads, tails and ABI stay
unchanged. For arbitrary seed, canonical sum is0 for total0, otherwise
1+(total-1) modulo(2^64-1); negative zero remains all ones. Carry1 implies the
last accumulator cannot already be all ones, so final ADC0 cannot overflow.
Independent integer-oracle corner/seeded checks and actual guarded guest tests
cover this property. Fewer instructions establish reduced work, not physical
cycles or a throughput guarantee; matched results are in the raw README.

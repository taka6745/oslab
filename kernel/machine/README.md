# Hand-encoded x86-64 hotpaths

`hotpath.asm` is project-authored machine code described by `db` and `dd`, with
symbolic branch displacements. The comments give the decoded instruction next to
each byte sequence; no compiler output or external implementation was imported.
Instruction encoding/semantics were consulted in the Intel SDM volume 2 and
string-operation guidance in the Intel optimization manuals:
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html

Four SysV AMD64 functions expose the same argument/result contracts as the
readable C: `machine_checksum`, `machine_transport_checksum`, `machine_memcpy`
and `machine_memset`. `GUEST_EXPORTS` also exports their original OS names.
Checksum loads an unaligned 64-bit word only with at least eight valid bytes; the main loop unrolls four
words after checking at least 32 valid bytes,
then uses bounded word/byte tails. ADD/ADC performs end-around carry at 64 bits;
32-bit and 16-bit folding plus byte rotation returns the network checksum.
This also avoids accumulator overflow for lengths above the IPv4 limit.
Transport rejects lengths above 65535 before touching the input and computes
its real pseudo-header from the supplied addresses, protocol and length.

Copy retains the existing REP MOVSQ plus bounded REP MOVSB tail algorithm;
fill uses REP STOSB. Both need the ABI direction flag clear, as required by the
kernel's existing C runtime. The copy contract follows memcpy: overlapping
ranges have no guaranteed behavior; identical buffers are supported. No SIMD,
external helpers or stack are needed. Code is 255 bytes (checksum entry/common
body 160, transport prelude 60, copy 21, fill 14). This is an experimental
selectable implementation, not a claim of faster physical hardware execution.

`tests/machine/hotpath.c` exercises the actual raw bytes with an independent
bytewise checksum oracle, all IPv4 lengths, 64 alignments, carry-heavy buffers,
protected ends and copy/fill canaries. Protected pages are necessary because
ASan does not instrument assembly accesses. `tests/machine/benchmark.c` links
the actual readable packet source under renamed symbols and alternates test
order. Rosetta measurements describe translated execution only; boot/wire
acceptance and the exact release image remain external osenv responsibilities.

Local Rosetta host oracle and ASan/UBSan driver tests passed. Removing the
64-bit carry instructions in an external test-only mutant caused oracle
rejection. The original host comparison allowed SIMD in readable C and is retained only
as an unmatched experiment: raw/C time ratio was 1.44 at 1460 bytes. The
corrected production-ISA comparison uses `-O3 -mgeneral-regs-only
-ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone`; disassembly
confirms no SIMD registers. Five alternating-order Rosetta rounds measured
median raw/C time ratios 0.634 at 20 bytes, 0.414 at 64 bytes, 0.626 at 1460
bytes and 0.670 at 65535 bytes (0-byte overhead ratio 1.290). This comparison
uses separately callable functions without guest LTO inlining; translated-host
measurements do not establish OS or physical performance.

`MACHINE_FILL_COMPACT` selects an 10-byte fill using PUSH RDI/POP RAX around
REP STOSB, saving four bytes with a temporary eight-byte stack slot. Default
fill remains stackless. Both variants passed protected-page/canary and
ASan/UBSan driver tests. `BENCH_FILL` selects fill in the host benchmark;
production-ISA readable fill beat either raw variant under Rosetta (raw/C
ratios roughly 2.02, including at 1460 bytes), with negligible default/compact
difference. Keep this variant optional pending actual guest measurements.

`branch.inc` defers signed rel8 range assertions until all targets resolve;
short jumps outside -128..127 fail assembly rather than silently wrapping.


`http.asm` supplies `machine_http_select(p,n,out)` and guest alias `http_select`;
`headers.asm` supplies `machine_headers_valid(p,line,end,http11)`. Both use SysV
AMD64 and support ELF64/MachO64 host objects. `MACHINE_HTTP=1` selects the raw
selector; `MACHINE_HEADERS=1` additionally selects the complete raw validator.
The original C parser/validator remain available as the readable alternative.
The response table is real C-authored wire data with actual lengths, exported as
`machine_http_responses[4]` in invalid/method/missing/page order. The raw selector
preserves callee-saved registers and aligns its stack before the validator call.

The selector rejects lengths above 768 before input access, scans only valid
four-byte windows for CRLFCRLF, and preserves output for incomplete requests.
A discovered separator bounds the two-byte request-line scan; completed requests
must end exactly at that separator plus four. It validates method, slash target
and HTTP version before calling the actual header validator. The validator checks
ASCII request-line bytes, token names, colons, value controls and whitespace,
case-insensitive field names, duplicate Host/Content-Length, nonempty Host without
internal whitespace, zero-only request length and HTTP/1.1 Host presence; it
rejects Transfer-Encoding. Header offsets are the selector's bounded offsets,
not an externally trusted arbitrary pointer interface.

These encodings are independently authored from Intel SDM Volume 2; the 128-bit
ASCII token bitmap and literal header names describe the HTTP grammar. Response
record layout is two native 64-bit fields (data pointer and byte length), matching
the authored C structure. No external guest code, generated instruction stream
or fabricated response is linked. Selector text is 290 bytes; validator text is
442 bytes plus 51 grammar/name bytes, for 783 bytes before alignment. The rest
of the kernel remains a mixture of readable C and selected hand-encoded routines.

The host gate compares actual response bytes with the retained readable parser,
including protected-page boundaries, all 256 byte values in grammar positions,
mixed-case/duplicate/whitespace cases and 200,000 seeded mutations. ASan/UBSan
host drivers passed; isolated generated-object mutants disabling the GET check
or HTTP/1.1 Host requirement were rejected. Complete guest wire/performance and
exact-image homelab acceptance remain pending.

T020 acceptance: real BIOS/fault/memory/recovery, packed-decoder, PVH boundary,
IRQ and clock gates passed; exact BIOS and corrected PVH production images
passed dedicated homelab wire and 1,000-request boundary/timeout checks.
BIOS raw kernel is 17,366 bytes versus 20,310; packed disk 14,848 versus 16,384.
Corrected PVH raw kernel is 18,326 versus 21,398; packed disk 15,872 versus
17,408. Default readable production remains byte-identical. Three alternating
TCG runs gave roughly comparable throughput: BIOS -1.2%, PVH +1.3%; neither
establishes a physical throughput improvement. PVH captured service was 20
versus 21 us; boot-to-first-response medians 19.82 versus 20.04 ms, below a
strong boot-gain claim at the harness resolution. Matched debug/no-LTO
instrumentation counted 45.1% fewer dispatched instructions during 1,000
responses; idle instruction work was unchanged. Independent totals agreed
exactly. This is debug dispatch accounting, not production cycles/cache data.
Native GP-only GCC checksum calls took 0.493 times readable C time at 1,460
bytes; no LTO or end-to-end physical server claim. Compact fill remains an
excluded experimental encoding: four saved bytes did not justify its slower
measured native call. Raw evidence, graphs and rejected candidates live in
external osenv local/t020, local/t020-native and local/t020-instructions.

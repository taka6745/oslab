# Literal network runtime

`network.inc` contains directly authored x86-64 hexadecimal instruction bytes;
no assembler, compiler, linker output or generated instruction selection is used.
`osenv.raw_build` writes bytes and applies fixed-width label fixups. Decode-only
NDISASM inspection checks instruction/register meaning; it supplies no guest bytes.
Project-authored `kernel/network.c` and `kernel/packets.c` provided the algorithmic
reference. Wire formats and state transitions were checked against primary
[RFC 791](https://www.rfc-editor.org/rfc/rfc791),
[RFC 826](https://www.rfc-editor.org/rfc/rfc826),
[RFC 2131](https://www.rfc-editor.org/rfc/rfc2131),
[RFC 2132](https://www.rfc-editor.org/rfc/rfc2132), and
[RFC 9293](https://www.rfc-editor.org/rfc/rfc9293).

`net_init()` zeros its arena and creates a live TSC-derived DHCP transaction ID.
`net_poll()` reads the real monotonic clock, advances DHCP, drains at most eight
NIC descriptors (releasing each), then advances one TCP connection. Calls follow
SysV64, preserve callee-saved GPRs and use no SIMD or red zone. Prerequisites are
identity-mapped low RAM, DF=0, initialized clock/NIC and the sibling authored
checksum, memory and HTTP routines. RX buffers remain owned until `nic_release`.
Transmit buffer 0x191000 is copied by the NIC driver before returning.

All state is in 0x190000..0x192fff: first page state, second page TX frame, third
page the 768-byte request buffer. Offset 0 stores sampled milliseconds; 8/9 store
configured/offered state; 12 transaction ID; 16 address; 20/24 offered address and
server; 28/32 subnet and router; 36 lease seconds; 40 expiry; 48 DHCP retry time.
Connection state starts at offset64: active/established/closing/peer-FIN flags,
remote IP68, remote port72, window74, MSS76, receive-next80, send-unacknowledged84,
send-next88, pending sequence92, flags96, retry count97, payload size100,
request size104, response offset108, data pointer112, length120, deadline128,
retry time136, and peer MAC144. Offsets160..271 are bounded parser/TX scratch.
Offsets280..375 hold one deferred SYN: pending flag280, header length284,
source/destination288/292, MAC296, up to60 header bytes304, expiry368.
While the old connection closes with its own FIN acknowledged, a different
tuple may occupy this slot. It cannot overwrite the old connection. After close,
the saved bytes re-enter the complete TCP parser; expiry, lease loss and NAK
clear the slot. This is a bounded resource policy, not concurrent serving.
IPs in state retain their four wire bytes; checksum arguments convert to host
integer order. TCP sequence arithmetic uses signed modulo-32-bit differences.

DHCP sends broadcast DISCOVER/REQUEST, accepts bounded options only with matching
transaction ID/MAC/cookie, rejects duplicate required options, and installs an ACK
only for the selected offer/server. The subnet must be contiguous, router in the
same subnet when the optional router is supplied, and lease nonzero. Matching
NAK restarts discovery; four unanswered REQUEST attempts return to discovery.
Expiry clears the active connection and resumes DHCP. It does not renew early. UDP's RFC-permitted zero checksum is used for the
outgoing IPv4 DHCP messages; nonzero incoming checksums are validated.

Ethernet accepts only our MAC or broadcast. IPv4 validates version, header and
total lengths, checksum, TTL, fragment offset/MF and destination. ARP replies only
to actual validated requests for the current leased IP, with matching Ethernet
and sender hardware addresses. TCP validates ports, destination, checksum,
header/options, matching peer, sequence and ACK bounds. MSS defaults536, rejects
zero/duplicates, and caps1460; responses respect MSS and the peer's window.
Requests are accumulated up to768; out-of-order data is ACKed for retransmission,
duplicate prefix bytes are trimmed, and unsupported excess/pipelined data resets
the connection. One outstanding segment supports five retries at250ms and a
10-second connection deadline. FIN consumes sequence space and must fit the
window. The peer's handshake MAC is retained for replies through the same hop.

The T022 exact packed image passed all 61 actual-guest boundary checks and real
Ethernet peer HTTP reconstruction on e1000 and e1000e, including segmented and
out-of-order requests, sequence wrap, zero windows and lost ACK/FIN retransmission.
The isolated missing IPv4 checksum rejection mutant is rejected. Local/homelab
results and current image hash are in [release measurements](README.md).

Reusable external boundary gate, from the osenv checkout:

```sh
python3 -m osenv.raw_network_test --build build/EXACT-IMAGE --output local/NEW.json
python3 -m osenv.raw_queue_test --build build/EXACT-IMAGE --output local/QUEUE.json
```

Peer tests use a realtime guest clock because their socket observation waits use
wall time. Under virtual icount sleep=off, a short host wait advanced the guest
past its valid 10-second connection deadline before the peer reopened its window.
Saved packets and guest memory establish that expiry; the harness correction
changed no guest deadline or assertion. Deterministic virtual CPU tests remain
separate (`osenv/local/t021/release-gate/network-clock-mismatch.json`).

T022 shortens proven in-range branches and cold DHCP addressing. The TCP receiver's
original hot instructions were retained after matched throughput tests rejected a
smaller addressing variant. The external writer validates every displacement;
independent disassembly and real-image tests check control flow and outcomes.
No packet checks or deadlines were removed. Candidate bytes and comparisons stay
outside this checkout under osenv/local/t022.

This is production-only: no outbound client, ICMP, DNS, IPv6, debug protocol,
keep-alive, parallel connections, TCP window scaling or early renewal.

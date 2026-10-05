; All instruction bytes authored directly; Intel SDM encoding and 82574 registers.
.section text
.org 100cb0
@clock_init
b9 00 00 d0 fe
48 8b 01
48 89 c2
48 c1 ea 20
48 85 d2
74 rel8:clock_bad
48 81 fa 80 96 98 00
75 rel8:clock_bad
a9 00 20 00 00
74 rel8:clock_bad
a8 ff
74 rel8:clock_bad
48 89 14 25 20 01 18 00
48 8b 41 10
48 83 e0 fc
48 89 41 10
48 c7 81 f0 00 00 00 00 00 00 00
48 83 c8 01
48 89 41 10
b8 01 00 00 00
c3
@clock_bad
31 c0
c3
@clock_ms
; Fixed10nsHPET validatedabove. floor(counter/100000), exactall64bitcounts.
b9 00 00 d0 fe
48 8b 81 f0 00 00 00
48 c1 e8 05
48 ba 43 78 b4 71 c4 5a 7c 0a
48 f7 e2
48 c1 ea 07
48 89 d0
c3
@pci_read
ba f8 0c 00 00
ef
66 ba fc 0c ; EDX upper16 remain zero from preceding MOV EDX,0xcf8
ed
c3
@pci_write
ba f8 0c 00 00
ef
89 c8
66 ba fc 0c ; EDX upper16 remain zero from preceding MOV EDX,0xcf8
ef
c3
@nic_init
; RBP is preserved MMIO base; avoids high-register/SIB bytes at disp32 offsets.
53 55 41 55
bb 00 01 18 00
31 ed
48 89 6b 08
e8 rel32:clock_init
85 c0
0f 84 rel32:nic_init_bad
41 b9 00 00 00 80
@nic_scan
44 89 c8
e8 rel32:pci_read
3d 86 80 0e 10
74 rel8:nic_found
3d 86 80 d3 10
74 rel8:nic_found
41 81 c1 00 08 00 00
41 81 f9 00 00 01 80
72 rel8:nic_scan
e9 rel32:nic_init_bad
@nic_found
44 89 c8
83 c8 10
e8 rel32:pci_read
a8 01
0f 85 rel32:nic_init_bad
89 c1
83 e1 06
85 c9
74 rel8:nic_bar_ok
83 f9 04
0f 85 rel32:nic_init_bad
41 89 c2
44 89 c8
83 c8 14
e8 rel32:pci_read
85 c0
0f 85 rel32:nic_init_bad
44 89 d0
@nic_bar_ok
83 e0 f0
3d 00 00 00 c0
0f 82 rel32:nic_init_bad
89 c5
44 89 c8
83 c8 3c
e8 rel32:pci_read
0f b6 c0
89 43 1c
44 89 c8
83 c8 04
e8 rel32:pci_read
0f b7 c8
81 e1 ff fb 00 00
83 c9 06
44 89 c8
83 c8 04
e8 rel32:pci_write
c7 85 d8 00 00 00 ff ff ff ff
31 c9 ; ECX scratch; reuse zero for disabled RCTL/TCTL dword stores
89 8d 00 01 00 00
89 8d 00 04 00 00
8b 45 00
0d 00 00 00 04
89 45 00
e8 rel32:clock_ms
4c 8d 68 64
@nic_reset_wait
f7 45 00 00 00 00 04
74 rel8:nic_reset_done
e8 rel32:clock_ms
4c 39 e8
0f 83 rel32:nic_init_bad
f3 90
eb rel8:nic_reset_wait
@nic_reset_done
c7 85 d8 00 00 00 ff ff ff ff
8b 85 c0 00 00 00
83 4d 00 40
8b 85 00 54 00 00
89 03
8b 85 04 54 00 00
66 89 43 04
a9 00 00 00 80
0f 84 rel32:nic_init_bad
f6 03 01
0f 85 rel32:nic_init_bad
0f b7 4b 04
0b 0b
0f 84 rel32:nic_init_bad
bf 00 10 18 00
31 c0
b9 20 00 00 00
f3 48 ab
bf 00 10 18 00
b8 00 20 18 00
ba 00 60 18 00
b9 08 00 00 00
@nic_ring_init
48 89 07
48 89 97 80 00 00 00
c6 87 8c 00 00 00 01
05 00 08 00 00
81 c2 00 08 00 00
83 c7 10
ff c9
75 rel8:nic_ring_init
31 c9
@nic_mta
c7 84 8d 00 52 00 00 00 00 00 00
ff c1
81 f9 80 00 00 00
72 rel8:nic_mta
31 c9 ; MTA loop done; shared zero for MMIO/state dword stores
c7 85 00 28 00 00 00 10 18 00
89 8d 04 28 00 00
c7 85 08 28 00 00 80 00 00 00
89 8d 10 28 00 00
c7 85 18 28 00 00 07 00 00 00
c7 85 00 38 00 00 80 10 18 00
89 8d 04 38 00 00
c7 85 08 38 00 00 80 00 00 00
89 8d 10 38 00 00
89 8d 18 38 00 00
c7 85 10 04 00 00 0a 20 60 00
c7 85 00 04 00 00 fa 00 04 00
c7 85 00 01 00 00 02 80 00 04
89 8d 20 28 00 00
89 8d 2c 28 00 00
c7 85 c4 00 00 00 40 00 00 00
89 4b 10
89 4b 14
89 4b 18
e8 rel32:clock_ms
4c 8d a8 b8 0b 00 00
@nic_link_wait
f7 45 08 02 00 00 00
75 rel8:nic_init_good
e8 rel32:clock_ms
4c 39 e8
73 rel8:nic_init_bad
f3 90
eb rel8:nic_link_wait
@nic_init_good
48 89 6b 08
b8 01 00 00 00
eb rel8:nic_init_end
@nic_init_bad
31 c0
@nic_init_end
41 5d 5d 5b c3
@nic_pending
8b 04 25 10 01 18 00
c1 e0 04
0f b6 80 0c 10 18 00
83 e0 01
c3
@nic_receive
83 3c 25 18 01 18 00 00
75 rel8:nic_rx_none
41 b8 08 00 00 00
@nic_rx_next
8b 0c 25 10 01 18 00
89 c8
c1 e0 04
48 8d b0 00 10 18 00
f6 46 0c 01
74 rel8:nic_rx_none
f6 46 0c 02
74 rel8:nic_rx_drop
80 7e 0d 00
75 rel8:nic_rx_drop
0f b7 56 08
83 fa 0e
72 rel8:nic_rx_drop
81 fa ea 05 00 00
77 rel8:nic_rx_drop
c7 04 25 18 01 18 00 01 00 00 00
89 c8
c1 e0 0b
48 05 00 20 18 00
c3
@nic_rx_drop
c7 04 25 18 01 18 00 01 00 00 00
48 83 ec 08
e8 rel32:nic_release
48 83 c4 08
41 ff c8 75 rel8:nic_rx_next
eb rel8:nic_rx_none
@nic_rx_none
31 c0
31 d2
c3
@nic_release
83 3c 25 18 01 18 00 00
74 rel8:nic_release_end
8b 0c 25 10 01 18 00
89 c8
c1 e0 04
c6 80 0c 10 18 00 00
0f ae f8
48 8b 14 25 08 01 18 00
89 8a 18 28 00 00
8b 42 08
ff c1
83 e1 07
89 0c 25 10 01 18 00
c7 04 25 18 01 18 00 00 00 00 00
@nic_release_end
c3
@nic_send
48 8b 14 25 08 01 18 00
48 85 d2
0f 84 rel32:nic_tx_bad
48 83 fe 0e
0f 82 rel32:nic_tx_bad
48 81 fe ea 05 00 00
0f 87 rel32:nic_tx_bad
44 8b 04 25 14 01 18 00
44 89 c0
c1 e0 04
4c 8d 88 80 10 18 00
41 f6 41 0c 01
74 rel8:nic_tx_bad
41 f6 41 0c 0e
75 rel8:nic_tx_bad
49 89 fa
48 89 f1
89 f0 ; bounded n<=1514; preserve original count in EAX
48 c1 e9 03 ; whole qwords only
49 8b 39
4c 89 d6
f3 48 a5 ; REP MOVSQ
89 c1
83 e1 07
f3 a4 ; remaining n modulo8 bytes
4c 29 d6
89 f0
83 f8 3c
73 rel8:nic_tx_length
b9 3c 00 00 00
29 c1
50
31 c0
f3 aa
58
b8 3c 00 00 00
@nic_tx_length
66 41 89 41 08
41 c7 41 0a 00 0b 00 00
66 41 c7 41 0e 00 00
0f ae f8
41 ff c0
41 83 e0 07
44 89 04 25 14 01 18 00
44 89 82 18 38 00 00
8b 42 08
b8 01 00 00 00
c3
@nic_tx_bad
31 c0
c3
@nic_interrupt
48 8b 04 25 08 01 18 00
48 85 c0
74 rel8:nic_interrupt_end
8b 80 c0 00 00 00
@nic_interrupt_end
c3
@nic_irq_enable
48 8b 04 25 08 01 18 00
48 85 c0
74 rel8:nic_interrupt_end
c7 80 d0 00 00 00 d4 00 00 00
8b 40 08
c3
@driver_end

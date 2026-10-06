; Authored literal x86-64 opcodes. See network.md for state layout/provenance.
.section text
.org 101190
@net_init
53 bb 00 00 19 00
48 89 df be 00 30 00 00 e8 rel32:mem_zero
0f 31 85 c0 75 05 b8 01 00 00 00 89 43 0c
5b c3
@net_poll
53 41 54 41 55 bb 00 00 19 00
 e8 rel32:clock_ms 48 89 03
80 7b 08 00 74 rel8:net_unconfigured
48 3b 43 28 72 rel8:net_receive
c6 43 08 00 c6 43 40 00 c6 43 09 00
c6 83 18 01 00 00 00
@net_unconfigured
48 3b 43 30 72 rel8:net_receive
48 05 e8 03 00 00 48 89 43 30
e8 rel32:net_dhcp_send
@net_receive
; at most eight RX descriptors per poll
41 bd 08 00 00 00
@net_rx_loop
e8 rel32:nic_receive 85 d2 74 rel8:net_rx_done
49 89 c4 89 d6
48 8b 03 48 89 04 25 28 01 18 00
4c 89 e7 e8 rel32:net_frame
e8 rel32:nic_release
41 ff cd 75 rel8:net_rx_loop
@net_rx_done
e8 rel32:net_server_poll
; Single bounded queued handshake; actual copied bytes, never an active second connection.
80 bb 18 01 00 00 00 74 rel8:net_poll_end
48 8b 03 48 3b 83 70 01 00 00 73 rel8:net_syn_queue_clear
80 7b 08 01 75 rel8:net_syn_queue_clear
48 3b 43 28 73 rel8:net_syn_queue_clear
80 7b 40 00 75 rel8:net_poll_end
c6 83 18 01 00 00 00
8b 83 20 01 00 00 89 83 a0 00 00 00
8b 83 24 01 00 00 89 83 a4 00 00 00
48 8d bb 30 01 00 00 8b b3 1c 01 00 00 48 8d 93 28 01 00 00
e8 rel32:net_tcp_receive
eb rel8:net_poll_end
@net_syn_queue_clear
c6 83 18 01 00 00 00
@net_poll_end
41 5d 41 5c 5b c3
; common IPv4 transmit: TX frame already populated, esi=transport length,
; edx=protocol, ecx=source (wire-order dword), r8d=destination wire-order
@net_ip_send
53 41 54 41 55 bb 00 00 19 00
41 bc 00 10 19 00
41 89 f5 41 83 c5 22
41 c7 44 24 0c 08 00 45 00
8d 46 14 66 c1 c0 08 66 41 89 44 24 10
41 c7 44 24 12 00 00 40 00
41 c6 44 24 16 40 41 88 54 24 17
41 c7 44 24 18 00 00 00 00
41 89 4c 24 1a 45 89 44 24 1e
49 8d 7c 24 06 be abs32:nic_mac ba 06 00 00 00 e8 rel32:mem_copy
49 8d 7c 24 0e be 14 00 00 00 e8 rel32:checksum
66 c1 c0 08 66 41 89 44 24 18
41 83 fd 3c 73 rel8:net_ip_queue
4b 8d 3c 2c be 3c 00 00 00 44 29 ee e8 rel32:mem_zero
41 bd 3c 00 00 00
@net_ip_queue
4c 89 e7 44 89 ee e8 rel32:nic_send
41 5d 41 5c 5b c3
@net_dhcp_send
53 41 54 48 83 ec 08 bb 00 00 19 00
41 bc 00 10 19 00
80 7b 09 00 74 rel8:net_dhcp_attempt_ready
80 7b 0a 04 72 rel8:net_dhcp_attempt_ready
c6 43 09 00 c6 43 0a 00
@net_dhcp_attempt_ready
fe 43 0a
4c 89 e7 be 58 01 00 00 e8 rel32:mem_zero
49 c7 04 24 ff ff ff ff 66 41 c7 44 24 04 ff ff
41 c7 44 24 22 00 44 00 43
41 c7 44 24 2a 01 01 06 00
8b 43 0c 0f c8 41 89 44 24 2e
66 41 c7 44 24 34 80 00
49 8d 7c 24 46 be abs32:nic_mac ba 06 00 00 00 e8 rel32:mem_copy
41 c7 84 24 16 01 00 00 63 82 53 63
41 c7 84 24 1a 01 00 00 35 01 01 37
41 c7 84 24 1e 01 00 00 03 01 03 33
41 c6 84 24 22 01 00 00 ff
be 01 01 00 00
80 7b 09 00 74 rel8:net_dhcp_options_done
41 c7 84 24 1a 01 00 00 35 01 03 32
66 41 c7 84 24 1e 01 00 00 04 00
8b 43 14 41 89 84 24 1f 01 00 00
66 41 c7 84 24 23 01 00 00 36 04
8b 43 18 41 89 84 24 25 01 00 00
41 c7 84 24 29 01 00 00 37 03 01 03
66 41 c7 84 24 2d 01 00 00 33 ff
be 0d 01 00 00
@net_dhcp_options_done
89 f0 66 c1 c0 08 66 41 89 44 24 26
ba 11 00 00 00 31 c9 41 b8 ff ff ff ff
e8 rel32:net_ip_send
48 83 c4 08 41 5c 5b c3
; Frame parser rdi frame, esi size. Preserves NIC RX pointer ownership.
@net_frame
53 55 41 54 41 55 41 56 41 57 48 83 ec 08
bb 00 00 19 00
49 89 fc 41 89 f5
83 fe 0e 0f 82 rel32:net_frame_end
; destination MAC: actual unicast or all-ones broadcast
8b 07 3b 04 25 abs32:nic_mac 75 rel8:net_mac_broadcast
66 8b 47 04 66 3b 04 25 abs32:nic_mac_tail 74 rel8:net_mac_ok
@net_mac_broadcast
41 83 3c 24 ff 0f 85 rel32:net_frame_end
66 41 83 7c 24 04 ff 0f 85 rel32:net_frame_end
@net_mac_ok
66 41 81 7c 24 0c 08 06 0f 84 rel32:net_arp_receive
66 41 81 7c 24 0c 08 00 0f 85 rel32:net_frame_end
41 83 fd 22 0f 82 rel32:net_frame_end
49 8d 6c 24 0e
0f b6 45 00 89 c2 83 e2 f0 83 fa 40 0f 85 rel32:net_frame_end
83 e0 0f c1 e0 02 83 f8 14 0f 82 rel32:net_frame_end
41 89 c6 41 8d 55 f2 39 d0 0f 87 rel32:net_frame_end
0f b7 45 02 66 c1 c0 08 0f b7 c0 39 d0 0f 87 rel32:net_frame_end
44 39 f0 0f 82 rel32:net_frame_end
41 89 c7 66 f7 45 06 3f ff 75 rel8:net_frame_end
80 7d 08 00 74 rel8:net_frame_end
48 89 ef 44 89 f6 e8 rel32:checksum 85 c0 75 rel8:net_frame_end
8b 45 0c 89 83 a0 00 00 00 8b 45 10 89 83 a4 00 00 00
80 7b 08 00 74 rel8:net_ip_destination_ok
3b 43 10 74 rel8:net_ip_destination_ok
83 f8 ff 75 rel8:net_frame_end
@net_ip_destination_ok
4a 8d 7c 35 00 45 29 f7 44 89 fe
80 7d 09 11 74 rel8:net_udp_receive
80 7d 09 06 75 rel8:net_frame_end
80 7b 08 00 74 rel8:net_frame_end
8b 83 a4 00 00 00 3b 43 10 75 rel8:net_frame_end
49 8d 54 24 06 e8 rel32:net_tcp_receive
@net_frame_end
48 83 c4 08 41 5f 41 5e 41 5d 41 5c 5d 5b c3
@net_udp_receive
83 fe 08 72 rel8:net_frame_end
0f b7 47 04 66 c1 c0 08 0f b7 c0 39 f0 75 rel8:net_frame_end
66 83 7f 06 00 74 rel8:net_udp_checksum_ok
49 89 fe 41 89 f7 48 89 f9 49 89 f0
8b bb a0 00 00 00 0f cf 8b b3 a4 00 00 00 0f ce ba 11 00 00 00
e8 rel32:transport_checksum
4c 89 f7 44 89 fe 85 c0 75 rel8:net_frame_end
@net_udp_checksum_ok
80 7b 08 00 75 rel8:net_frame_end
81 3f 00 43 00 44 0f 85 rel32:net_frame_end
48 83 c7 08 83 ee 08 e8 rel32:net_dhcp_receive
e9 rel32:net_frame_end
; ARP reply only to validated requests for our active address.
@net_arp_receive
41 83 fd 2a 0f 82 rel32:net_frame_end
80 7b 08 00 0f 84 rel32:net_frame_end
41 81 7c 24 0e 00 01 08 00 0f 85 rel32:net_frame_end
41 81 7c 24 12 06 04 00 01 0f 85 rel32:net_frame_end
41 8b 44 24 26 3b 43 10 0f 85 rel32:net_frame_end
41 8b 44 24 06 41 3b 44 24 16 0f 85 rel32:net_frame_end
66 41 8b 44 24 0a 66 41 3b 44 24 1a 0f 85 rel32:net_frame_end
41 f6 44 24 06 01 0f 85 rel32:net_frame_end
bf 00 10 19 00 be 3c 00 00 00 e8 rel32:mem_zero
bf 00 10 19 00 49 8d 74 24 06 ba 06 00 00 00 e8 rel32:mem_copy
bf 06 10 19 00 be abs32:nic_mac ba 06 00 00 00 e8 rel32:mem_copy
48 b8 06 08 00 01 08 00 06 04 48 a3 0c 10 19 00 00 00 00 00
66 c7 04 25 14 10 19 00 00 02
bf 16 10 19 00 be abs32:nic_mac ba 06 00 00 00 e8 rel32:mem_copy
8b 43 10 89 04 25 1c 10 19 00
bf 20 10 19 00 49 8d 74 24 16 ba 06 00 00 00 e8 rel32:mem_copy
41 8b 44 24 1c 89 04 25 26 10 19 00
bf 00 10 19 00 be 3c 00 00 00 e8 rel32:nic_send
e9 rel32:net_frame_end
; RBP=state+256 shortens scratch addresses; preserve RBP and SysV call alignment.
@net_dhcp_receive
53 55 41 54 41 55 41 56 41 57 48 83 ec 08
bb 00 00 19 00 bd 00 01 19 00
49 89 fc 41 89 f5
81 fe f0 00 00 00 0f 82 rel32:net_dhcp_end
81 3f 02 01 06 00 0f 85 rel32:net_dhcp_end
8b 47 04 0f c8 3b 43 0c 0f 85 rel32:net_dhcp_end
8b 47 1c 3b 04 25 abs32:nic_mac 0f 85 rel32:net_dhcp_end
66 8b 47 20 66 3b 04 25 abs32:nic_mac_tail 0f 85 rel32:net_dhcp_end
81 bf ec 00 00 00 63 82 53 63 0f 85 rel32:net_dhcp_end
8b 47 10 89 45 f0
48 8d 7d dc be 14 00 00 00 e8 rel32:mem_zero
c7 45 f4 00 00 00 00
4d 8d b4 24 f0 00 00 00 4f 8d 3c 2c
@net_dhcp_option
4d 39 fe 0f 83 rel32:net_dhcp_end
45 0f b6 06 49 ff c6 45 85 c0 74 rel8:net_dhcp_option
41 80 f8 ff 0f 84 rel32:net_dhcp_option_end
4d 39 fe 0f 83 rel32:net_dhcp_end
45 0f b6 0e 49 ff c6 4b 8d 04 0e 4c 39 f8 0f 87 rel32:net_dhcp_end
41 83 f8 35 74 rel8:net_dhcp_msg
41 83 f8 01 74 rel8:net_dhcp_mask
41 83 f8 03 74 rel8:net_dhcp_router
41 83 f8 33 0f 84 rel32:net_dhcp_lease
41 83 f8 36 0f 84 rel32:net_dhcp_server
@net_dhcp_next
4d 01 ce eb rel8:net_dhcp_option
@net_dhcp_msg
41 83 f9 01 0f 85 rel32:net_dhcp_end
f6 45 f4 01 0f 85 rel32:net_dhcp_end
80 4d f4 01 41 8a 06 88 83 dc 00 00 00
eb rel8:net_dhcp_next
@net_dhcp_mask
41 83 f9 04 0f 85 rel32:net_dhcp_end
f6 45 f4 02 0f 85 rel32:net_dhcp_end
80 4d f4 02 41 8b 06 89 45 e0
eb rel8:net_dhcp_next
@net_dhcp_router
41 83 f9 04 0f 82 rel32:net_dhcp_end
41 f6 c1 03 0f 85 rel32:net_dhcp_end
f6 45 f4 04 0f 85 rel32:net_dhcp_end
80 4d f4 04 41 8b 06 89 45 e4
eb rel8:net_dhcp_next
@net_dhcp_lease
41 83 f9 04 0f 85 rel32:net_dhcp_end
f6 45 f4 08 0f 85 rel32:net_dhcp_end
80 4d f4 08 41 8b 06 0f c8 89 45 e8
e9 rel32:net_dhcp_next
@net_dhcp_server
41 83 f9 04 0f 85 rel32:net_dhcp_end
f6 45 f4 10 0f 85 rel32:net_dhcp_end
80 4d f4 10 41 8b 06 89 45 ec
e9 rel32:net_dhcp_next
@net_dhcp_option_end
8b 45 f4 83 e0 11 83 f8 11 0f 85 rel32:net_dhcp_end
8b 45 ec 85 c0 0f 84 rel32:net_dhcp_end
80 7d dc 06 0f 84 rel32:net_dhcp_nak
8b 55 f0 85 d2 0f 84 rel32:net_dhcp_end
0f ca c1 ea 18 85 d2 0f 84 rel32:net_dhcp_end
83 fa 7f 0f 84 rel32:net_dhcp_end
80 fa e0 0f 83 rel32:net_dhcp_end
80 7d dc 02 0f 84 rel32:net_dhcp_offer
80 7d dc 05 0f 85 rel32:net_dhcp_end
80 7b 09 01 0f 85 rel32:net_dhcp_end
3b 43 18 0f 85 rel32:net_dhcp_end
8b 45 f0 3b 43 14 0f 85 rel32:net_dhcp_end
; ACK requires subnet and lease; router option is optional. Validate mask.
8b 45 f4 83 e0 1b 83 f8 1b 0f 85 rel32:net_dhcp_end
8b 45 e0 85 c0 0f 84 rel32:net_dhcp_end
0f c8 f7 d0 8d 50 01 85 d0 0f 85 rel32:net_dhcp_end
8b 45 e4 85 c0 74 rel8:net_dhcp_route_valid
33 45 f0 23 45 e0 0f 85 rel32:net_dhcp_end
@net_dhcp_route_valid
8b 45 e8 85 c0 0f 84 rel32:net_dhcp_end
48 69 c0 e8 03 00 00 48 03 03 72 rel8:net_dhcp_end
48 89 43 28
8b 45 f0 89 43 10
8b 45 e0 89 43 1c
8b 45 e4 89 43 20
8b 45 e8 89 43 24
c6 43 08 01 eb rel8:net_dhcp_end
@net_dhcp_offer
80 7b 09 00 75 rel8:net_dhcp_end
89 43 18 8b 45 f0 89 43 14 c6 43 09 01 c6 43 0a 00
48 c7 43 30 00 00 00 00
eb rel8:net_dhcp_end
@net_dhcp_nak
80 7b 09 01 75 rel8:net_dhcp_end
3b 43 18 75 rel8:net_dhcp_end
c6 83 18 01 00 00 00
c6 43 09 00 c6 43 0a 00 48 c7 43 30 00 00 00 00
@net_dhcp_end
48 83 c4 08 41 5f 41 5e 41 5d 41 5c 5d 5b c3
; TCP builder edi flags, esi sequence, rdx payload, ecx size.
@net_tcp_send
53 41 54 41 55 41 56 41 57
bb 00 00 19 00
41 89 fd 41 89 f6 49 89 d7 89 8b 04 01 00 00
41 bc 00 10 19 00
4c 89 e7 be 3c 00 00 00 e8 rel32:mem_zero
4c 89 e7 48 8d b3 90 00 00 00 ba 06 00 00 00 e8 rel32:mem_copy
66 41 c7 44 24 22 00 50 0f b7 43 48 66 41 89 44 24 24
44 89 f0 0f c8 41 89 44 24 26
8b 43 50 0f c8 41 89 44 24 2a
41 c6 44 24 2e 50 45 88 6c 24 2f
66 41 c7 44 24 30 03 00
41 be 14 00 00 00 41 f6 c5 02 74 rel8:net_tcp_header_done
41 c6 44 24 2e 60 41 c7 44 24 36 02 04 05 b4
41 be 18 00 00 00
@net_tcp_header_done
49 8d 7c 24 22 4c 01 f7 4c 89 fe 8b 93 04 01 00 00
e8 rel32:mem_copy
44 03 b3 04 01 00 00
8b 7b 10 0f cf 8b 73 44 0f ce ba 06 00 00 00 49 8d 4c 24 22 45 89 f0
e8 rel32:transport_checksum
66 c1 c0 08 66 41 89 44 24 32
44 89 f6 ba 06 00 00 00 8b 4b 10 44 8b 43 44 e8 rel32:net_ip_send
41 5f 41 5e 41 5d 41 5c 5b c3
; Retransmittable segment begins at snd_next and consumes sequence space.
@net_transmit
53 bb 00 00 19 00
8b 73 58 89 73 5c 40 88 7b 60 c6 43 61 00 89 4b 64
89 c8 40 f6 c7 03 74 rel8:net_transmit_no_control
ff c0
@net_transmit_no_control
01 43 58 48 8b 03 48 05 fa 00 00 00 48 89 83 88 00 00 00
e8 rel32:net_tcp_send
5b c3
; TCP receiver rdi segment, esi bytes, rdx source Ethernet address.
@net_tcp_receive
53 41 54 41 55 41 56 41 57
bb 00 00 19 00
49 89 fc 41 89 f5 48 89 93 08 01 00 00
83 fe 14 0f 82 rel32:net_tcp_end
66 81 7f 02 00 50 0f 85 rel32:net_tcp_end
66 83 3f 00 0f 84 rel32:net_tcp_end
0f b6 47 0c c1 e8 04 c1 e0 02 83 f8 14 0f 82 rel32:net_tcp_end
39 f0 0f 87 rel32:net_tcp_end
41 89 c6 48 89 f9 49 89 f0
8b bb a0 00 00 00 0f cf 8b b3 a4 00 00 00 0f ce ba 06 00 00 00
e8 rel32:transport_checksum 85 c0 0f 85 rel32:net_tcp_end
41 8b 44 24 04 0f c8 89 83 b4 00 00 00
41 8b 44 24 08 0f c8 89 83 b8 00 00 00
41 0f b6 44 24 0d 89 83 bc 00 00 00
80 7b 40 00 0f 85 rel32:net_tcp_existing
83 e0 17 83 f8 02 0f 85 rel32:net_tcp_end
45 39 f5 0f 85 rel32:net_tcp_end
48 8b 93 08 01 00 00 f6 02 01 0f 85 rel32:net_tcp_end
; valid source IPv4 must be unicast and nonzero
8b 83 a0 00 00 00 85 c0 0f 84 rel32:net_tcp_end
0f c8 c1 e8 18 3c e0 0f 83 rel32:net_tcp_end
66 c7 43 4c 18 02 c6 83 fc 00 00 00 00
49 8d 54 24 14 4b 8d 34 34
@net_tcp_option
48 39 f2 0f 83 rel32:net_tcp_syn_valid
0f b6 02 48 ff c2 85 c0 74 rel8:net_tcp_syn_valid
83 f8 01 74 rel8:net_tcp_option
48 39 f2 0f 83 rel32:net_tcp_end
0f b6 0a 83 f9 02 0f 82 rel32:net_tcp_end
48 8d 7c 0a ff 48 39 f7 0f 87 rel32:net_tcp_end
83 f8 02 75 rel8:net_tcp_option_advance
83 f9 04 0f 85 rel32:net_tcp_end
80 bb fc 00 00 00 00 0f 85 rel32:net_tcp_end
c6 83 fc 00 00 00 01
0f b7 42 01 66 c1 c0 08 66 85 c0 0f 84 rel32:net_tcp_end
66 3d b4 05 76 rel8:net_tcp_mss_store
b8 b4 05 00 00
@net_tcp_mss_store
66 89 43 4c
@net_tcp_option_advance
48 89 fa e9 rel32:net_tcp_option
@net_tcp_syn_valid
c6 43 40 01 c6 43 41 00 c6 43 42 00 c6 43 43 00
8b 83 a0 00 00 00 89 43 44
66 41 8b 04 24 66 89 43 48
66 41 8b 44 24 0e 66 c1 c0 08 66 89 43 4a
8b 83 b4 00 00 00 ff c0 89 43 50
0f 31 89 43 54 89 43 58
c7 43 68 00 00 00 00 c7 43 6c 00 00 00 00
48 c7 43 70 00 00 00 00 48 c7 43 78 00 00 00 00
48 8b 03 48 05 10 27 00 00 48 89 83 80 00 00 00
48 8d bb 90 00 00 00 48 8b b3 08 01 00 00 ba 06 00 00 00 e8 rel32:mem_copy
bf 12 00 00 00 31 d2 31 c9 e8 rel32:net_transmit
e9 rel32:net_tcp_end
@net_tcp_existing
8b 83 a0 00 00 00 3b 43 44 0f 85 rel32:net_tcp_queue_syn
66 41 8b 04 24 66 3b 43 48 0f 85 rel32:net_tcp_queue_syn
f6 83 bc 00 00 00 04 74 rel8:net_tcp_not_reset
8b 83 b4 00 00 00 3b 43 50 0f 85 rel32:net_tcp_end
c6 43 40 00 e9 rel32:net_tcp_end
@net_tcp_not_reset
f6 83 bc 00 00 00 02 74 rel8:net_tcp_not_syn
80 7b 41 00 0f 85 rel32:net_tcp_end
8b 83 b4 00 00 00 ff c0 3b 43 50 0f 85 rel32:net_tcp_end
bf 12 00 00 00 8b 73 5c 31 d2 31 c9 e8 rel32:net_tcp_send
e9 rel32:net_tcp_end
@net_tcp_not_syn
f6 83 bc 00 00 00 10 0f 84 rel32:net_tcp_end
8b 83 b8 00 00 00 89 c2 2b 53 54 0f 88 rel32:net_tcp_end
89 c2 2b 53 58 0f 8f rel32:net_tcp_end
80 7b 41 00 75 rel8:net_tcp_established
3b 43 58 0f 85 rel32:net_tcp_end
8b 93 b4 00 00 00 3b 53 50 0f 85 rel32:net_tcp_end
c6 43 41 01
@net_tcp_established
89 43 54 66 41 8b 44 24 0e 66 c1 c0 08 66 89 43 4a
45 89 ef 45 29 f7 4d 01 f4
8b 83 b4 00 00 00 89 c2 2b 53 50 79 rel8:net_tcp_overlap_done
8b 53 50 29 c2 44 39 fa 0f 87 rel32:net_tcp_ack
49 01 d4 41 29 d7 01 d0
@net_tcp_overlap_done
3b 43 50 0f 85 rel32:net_tcp_ack
45 85 ff 74 rel8:net_tcp_fin
48 83 7b 70 00 0f 85 rel32:net_tcp_abort
8b 43 68 05 00 fd ff ff f7 d8 44 39 f8 0f 82 rel32:net_tcp_abort
; remaining request capacity = 768-request_size
bf 00 20 19 00 8b 43 68 48 01 c7
4c 89 e6 44 89 fa e8 rel32:mem_copy
44 01 7b 68 44 01 7b 50
bf 00 20 19 00 8b 73 68 e8 rel32:http_select
48 85 c0 74 rel8:net_tcp_fin
48 8b 10 48 89 53 70 48 8b 50 08 48 89 53 78
@net_tcp_fin
f6 83 bc 00 00 00 01 74 rel8:net_tcp_ack_normal
80 7b 43 00 75 rel8:net_tcp_ack_normal
ff 43 50 c6 43 43 01
@net_tcp_ack_normal
45 85 ff 75 rel8:net_tcp_ack_needed
f6 83 bc 00 00 00 01 74 rel8:net_tcp_close_check
@net_tcp_ack_needed
48 83 7b 70 00 74 rel8:net_tcp_ack
8b 43 54 3b 43 58 75 rel8:net_tcp_ack
66 83 7b 4a 00 74 rel8:net_tcp_ack
80 7b 42 00 75 rel8:net_tcp_ack
eb rel8:net_tcp_close_check
@net_tcp_ack
bf 10 00 00 00 8b 73 58 31 d2 31 c9 e8 rel32:net_tcp_send
@net_tcp_close_check
80 7b 42 00 74 rel8:net_tcp_end
80 7b 43 00 74 rel8:net_tcp_end
8b 43 54 3b 43 58 75 rel8:net_tcp_end
c6 43 40 00 eb rel8:net_tcp_end
@net_tcp_abort
bf 14 00 00 00 8b 73 58 31 d2 31 c9 e8 rel32:net_tcp_send
c6 43 40 00
@net_tcp_end
41 5f 41 5e 41 5d 41 5c 5b c3
@net_tcp_queue_syn
80 7b 42 01 0f 85 rel32:net_tcp_end
8b 43 54 3b 43 58 0f 85 rel32:net_tcp_end
8b 83 bc 00 00 00 83 e0 17 83 f8 02 0f 85 rel32:net_tcp_end
45 39 f5 0f 85 rel32:net_tcp_end
48 8b 93 08 01 00 00 f6 02 01 0f 85 rel32:net_tcp_end
8b 83 a0 00 00 00 85 c0 0f 84 rel32:net_tcp_end
0f c8 c1 e8 18 3c e0 0f 83 rel32:net_tcp_end
80 bb 18 01 00 00 00 0f 85 rel32:net_tcp_end
8b 83 a0 00 00 00 89 83 20 01 00 00
8b 83 a4 00 00 00 89 83 24 01 00 00
44 89 b3 1c 01 00 00
48 8b 03 48 05 10 27 00 00 48 89 83 70 01 00 00
48 8d bb 30 01 00 00 4c 89 e6 44 89 f2 e8 rel32:mem_copy
48 8d bb 28 01 00 00 48 8b b3 08 01 00 00 ba 06 00 00 00 e8 rel32:mem_copy
c6 83 18 01 00 00 01
e9 rel32:net_tcp_end
@net_server_poll
53 bb 00 00 19 00
80 7b 40 00 0f 84 rel32:net_server_end
48 8b 03 48 3b 83 80 00 00 00 0f 83 rel32:net_server_abort
48 3b 43 28 0f 83 rel32:net_server_clear
8b 53 54 3b 53 58 74 rel8:net_server_acked
48 3b 83 88 00 00 00 0f 82 rel32:net_server_end
fe 43 61 80 7b 61 05 0f 87 rel32:net_server_abort
48 05 fa 00 00 00 48 89 83 88 00 00 00
0f b6 7b 60 8b 73 5c 8b 4b 64 31 d2 85 c9 74 rel8:net_server_resend
48 8b 53 70 8b 43 6c 48 01 c2
@net_server_resend
e8 rel32:net_tcp_send e9 rel32:net_server_end
@net_server_acked
8b 43 64 01 43 6c c7 43 64 00 00 00 00
80 7b 41 00 0f 84 rel32:net_server_end
48 83 7b 70 00 0f 84 rel32:net_server_end
8b 43 6c 48 3b 43 78 73 rel8:net_server_finished
80 7b 42 00 0f 85 rel32:net_server_end
48 8b 4b 78 48 29 c1 0f b7 53 4c 48 39 d1 76 rel8:net_server_mss
89 d1
@net_server_mss
0f b7 53 4a 39 d1 76 rel8:net_server_window
89 d1
@net_server_window
85 c9 0f 84 rel32:net_server_end
bf 18 00 00 00 8d 34 08 48 3b 73 78 75 rel8:net_server_payload
39 d1 73 rel8:net_server_payload
83 cf 01 c6 43 42 01
@net_server_payload
48 8b 53 70 48 01 c2 e8 rel32:net_transmit
eb rel8:net_server_end
@net_server_finished
80 7b 42 00 75 rel8:net_server_closed
66 83 7b 4a 00 74 rel8:net_server_end
c6 43 42 01 bf 11 00 00 00 31 d2 31 c9 e8 rel32:net_transmit
eb rel8:net_server_end
@net_server_closed
80 7b 43 00 74 rel8:net_server_end
eb rel8:net_server_clear
@net_server_abort
bf 14 00 00 00 8b 73 58 31 d2 31 c9 e8 rel32:net_tcp_send
@net_server_clear
c6 43 40 00
@net_server_end
5b c3
.section bss
.org 190000
@net_state
.zero 4096
@net_tx_frame
.zero 4096
@net_request
.zero 4096

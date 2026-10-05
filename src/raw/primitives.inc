; ISA bytes authored in this project; no assembler/compiler output.
.section text
.org 100170
@transport_checksum
49 81 f8
ff ff 00 00
76 rel8:transport_checksum_valid
b8
01 00 00 00
c3
@transport_checksum_valid
89 f8
c1 ef 10
25
ff ff 00 00
01 f8
89 f7
c1 ef 10
81 e6
ff ff 00 00
01 f8 01 f0
0f b6 d2
01 d0
44 01 c0
0f c8
48 89 cf
4c 89 c6
eb rel8:sum
@checksum
31 c0
@sum
48 83 fe 20
72 rel8:eight
@sum_blocks
; ADD begins a carry chain; ADC consumes each preceding carry in the next word.
; CF=1 implies RAX<=UINT64_MAX-1, so final ADC0 cannot overflow.
48 03 07
48 13 47 08
48 13 47 10
48 13 47 18
48 83 d0 00
48 83 c7 20
48 83 ee 20
48 83 fe 20
73 rel8:sum_blocks
@eight
48 83 fe 08
72 rel8:tail
@eight_words
48 03 07
48 83 d0 00
48 83 c7 08
48 83 ee 08
48 83 fe 08
73 rel8:eight_words
@tail
48 83 fe 02
72 rel8:odd
@tail_pairs
0f b7 17
48 01 d0
48 83 d0 00
48 83 c7 02
48 83 ee 02
48 83 fe 02
73 rel8:tail_pairs
@odd
48 85 f6
74 rel8:fold
0f b6 17
48 01 d0
48 83 d0 00
@fold
48 89 c2
48 c1 ea 20
01 d0
83 d0 00
89 c2
c1 ea 10
66 01 d0
66 83 d0 00
66 c1 c0 08
66 f7 d0
0f b7 c0
c3
@mem_copy
48 89 f8 48 89 d1 48 c1 e9 03 f3 48 a5 89 d1 83 e1 07 f3 a4 c3
@mem_zero
48 89 f1 48 89 f2 ; RCX=count, RDX=original full-width size
48 c1 e9 03
31 c0
f3 48 ab ; REP STOSQ whole qwords
89 d1 83 e1 07
f3 aa c3 ; REP STOSB tail, RAX remains zero
@http_select
48 81 fe
00 03 00 00
76 rel8:http_bounded
48 8d 05
rel32:http_responses
c3
@http_bounded
31 c0
48 83 fe 04
72 rel8:http_incomplete
48 83 ee 04
@http_scan
81 3c 07
0d 0a 0d 0a
74 rel8:http_complete
48 ff c0 48 39 f0
76 rel8:http_scan
@http_incomplete
31 c0 c3
@http_complete
53 55 41 54 41 55 41 56
48 89 fb
31 ed
48 39 f0
0f 85 rel32:http_selected
49 89 c5
45 31 f6
@http_line_scan
66 42 81 3c 33
0d 0a
74 rel8:http_line_found
49 ff c6
eb rel8:http_line_scan
@http_line_found
31 c0
@http_first_space
4c 39 f0
0f 83 rel32:http_invalid
80 3c 03 20
74 rel8:http_first_found
48 ff c0
eb rel8:http_first_space
@http_first_found
48 85 c0
0f 84 rel32:http_invalid
48 8d 48 01
@http_second_space
4c 39 f1
0f 83 rel32:http_invalid
80 3c 0b 20
74 rel8:http_second_found
48 ff c1
eb rel8:http_second_space
@http_second_found
80 7c 03 01 2f
0f 85 rel32:http_invalid
4c 89 f2 48 29 ca
83 fa 09
0f 85 rel32:http_invalid
81 7c 0b 01
48 54 54 50
75 rel8:http_invalid
81 7c 0b 04
50 2f 31 2e
75 rel8:http_invalid
0f b6 54 0b 08
83 ea 30 83 fa 01
77 rel8:http_invalid
89 d7
bd
01 00 00 00
83 f8 04
75 rel8:http_get_test
81 3b
48 45 41 44
75 rel8:http_validate
bd
04 00 00 00
48 29 c1
83 f9 02
75 rel8:http_validate
ff c5
eb rel8:http_validate
@http_get_test
83 f8 03
75 rel8:http_validate
81 3b
47 45 54 20
75 rel8:http_validate
bd
02 00 00 00
48 29 c1
83 f9 02
75 rel8:http_validate
ff c5
@http_validate
89 f9 48 89 df
4c 89 f6 4c 89 ea
e8
rel32:headers_valid
84 c0
75 rel8:http_selected
@http_invalid
31 ed
@http_selected
48 8d 05
rel32:http_responses
c1 e5 04 48 01 e8
41 5e 41 5d 41 5c 5d 5b c3
@headers_valid
53 55 89 cd
31 c0
@http_ascii
48 39 f0
73 rel8:http_ascii_done
0f b6 1c 07
83 eb 20 83 fb 5e
0f 87 rel32:http_bad
48 ff c0
eb rel8:http_ascii
@http_ascii_done
4c 8d 46 02
@http_header
49 39 d0
0f 83 rel32:http_done
4d 89 c1
@http_stop_scan
49 39 d1
73 rel8:http_stop_found
66 42 81 3c 0f
0d 0a
74 rel8:http_stop_found
49 ff c1
eb rel8:http_stop_scan
@http_stop_found
4d 89 c2
@http_colon_scan
4d 39 ca
0f 83 rel32:http_bad
42 0f b6 04 17
3c 3a
74 rel8:http_colon_found
3c 7f
0f 87 rel32:http_bad
48 0f a3 05
rel32:http_tokens
0f 83 rel32:http_bad
49 ff c2
eb rel8:http_colon_scan
@http_colon_found
4d 39 c2
0f 84 rel32:http_bad
4d 8d 5a 01 4c 89 ce
@http_trim_first
49 39 f3
73 rel8:http_trim_last
42 80 3c 1f 20
74 rel8:http_first_inc
42 80 3c 1f 09
75 rel8:http_trim_last
@http_first_inc
49 ff c3
eb rel8:http_trim_first
@http_trim_last
4c 39 de
76 rel8:http_value_start
80 7c 37 ff 20
74 rel8:http_last_dec
80 7c 37 ff 09
75 rel8:http_value_start
@http_last_dec
48 ff ce
eb rel8:http_trim_last
@http_value_start
4c 89 d8
@http_value_scan
48 39 f0
73 rel8:http_value_done
0f b6 1c 07
83 fb 09
74 rel8:http_value_next
83 eb 20 83 fb 5e
0f 87 rel32:http_bad
@http_value_next
48 ff c0
eb rel8:http_value_scan
@http_value_done
4c 89 d0 4c 29 c0
83 f8 04
75 rel8:http_try_transfer
48 8d 1d rel32:http_host_name
b9
04 00 00 00
e8 rel32:http_named
0f 85 rel32:http_next_header
40 f6 c5 02
0f 85 rel32:http_bad
49 39 f3
0f 84 rel32:http_bad
83 cd 02
4c 89 d8
@http_host_scan
80 3c 07 20
74 rel8:http_bad
80 3c 07 09
74 rel8:http_bad
48 ff c0 48 39 f0
72 rel8:http_host_scan
eb rel8:http_next_header
@http_try_transfer
83 f8 11
75 rel8:http_try_length
48 8d 1d rel32:http_transfer_name
b9
11 00 00 00
e8 rel32:http_named
74 rel8:http_bad
eb rel8:http_next_header
@http_try_length
83 f8 0e
75 rel8:http_next_header
48 8d 1d rel32:http_length_name
b9
0e 00 00 00
e8 rel32:http_named
75 rel8:http_next_header
40 f6 c5 04
75 rel8:http_bad
49 39 f3
74 rel8:http_bad
83 cd 04
4c 89 d8
@http_zero_scan
80 3c 07 30
75 rel8:http_bad
48 ff c0 48 39 f0
72 rel8:http_zero_scan
@http_next_header
4d 8d 41 02
e9
rel32:http_header
@http_done
40 f6 c5 01
74 rel8:http_good
40 f6 c5 02
74 rel8:http_bad
@http_good
b8
01 00 00 00
5d 5b c3
@http_bad
31 c0 5d 5b c3
@http_named
42 8a 04 07
0c 20 3a 03
75 rel8:http_name_return
49 ff c0 48 ff c3 ff c9
75 rel8:http_named
@http_name_return
c3
@http_tokens
00 00 00 00 fa 6c ff 03 fe ff ff c7 ff ff ff 57
@http_host_name
68 6f 73 74
@http_transfer_name
74 72 61 6e 73 66 65 72 2d 65 6e 63 6f 64 69 6e 67
@http_length_name
63 6f 6e 74 65 6e 74 2d 6c 65 6e 67 74 68
.align 8
@http_responses
abs64:http_response_0
4f 00 00 00 00 00 00 00
abs64:http_response_1
69 00 00 00 00 00 00 00
abs64:http_response_2
4b 00 00 00 00 00 00 00
abs64:http_response_3
b4 05 00 00 00 00 00 00
abs64:http_response_2
41 00 00 00 00 00 00 00
abs64:http_response_3
5e 00 00 00 00 00 00 00
@http_response_0
48 54 54 50 2f 31 2e 30 20 34 30 30 20 42 61 64 20 52 65 71 75 65 73 74
0d 0a 43 6f 6e 74 65 6e 74 2d 4c 65 6e 67 74 68 3a 30 30 31 32 0d 0a 43
6f 6e 6e 65 63 74 69 6f 6e 3a 63 6c 6f 73 65 0d 0a 0d 0a 42 61 64 20 72
65 71 75 65 73 74 0a
@http_response_1
48 54 54 50 2f 31 2e 30 20 34 30 35 20 4d 65 74 68 6f 64 20 4e 6f 74 20
41 6c 6c 6f 77 65 64 0d 0a 41 6c 6c 6f 77 3a 20 47 45 54 2c 20 48 45 41
44 0d 0a 43 6f 6e 74 65 6e 74 2d 4c 65 6e 67 74 68 3a 30 30 31 33 0d 0a
43 6f 6e 6e 65 63 74 69 6f 6e 3a 63 6c 6f 73 65 0d 0a 0d 0a 47 45 54 20
72 65 71 75 69 72 65 64 0a
@http_response_2
48 54 54 50 2f 31 2e 30 20 34 30 34 20 4e 6f 74 20 46 6f 75 6e 64 0d 0a
43 6f 6e 74 65 6e 74 2d 4c 65 6e 67 74 68 3a 30 30 31 30 0d 0a 43 6f 6e
6e 65 63 74 69 6f 6e 3a 63 6c 6f 73 65 0d 0a 0d 0a 4e 6f 74 20 66 6f 75
6e 64 0a
@http_response_3
48 54 54 50 2f 31 2e 30 20 32 30 30 20 0d 0a 43 6f 6e 74 65 6e 74 2d 54
79 70 65 3a 74 65 78 74 2f 68 74 6d 6c 3b 63 68 61 72 73 65 74 3d 75 74
66 2d 38 0d 0a 43 6f 6e 74 65 6e 74 2d 4c 65 6e 67 74 68 3a 31 33 36 36
0d 0a 43 6f 6e 6e 65 63 74 69 6f 6e 3a 63 6c 6f 73 65 0d 0a 0d 0a 3c 21
64 6f 63 74 79 70 65 20 68 74 6d 6c 3e 3c 68 74 6d 6c 20 6c 61 6e 67 3d
65 6e 3e 3c 6d 65 74 61 20 63 68 61 72 73 65 74 3d 75 74 66 2d 38 3e 3c
6d 65 74 61 20 6e 61 6d 65 3d 76 69 65 77 70 6f 72 74 20 63 6f 6e 74 65
6e 74 3d 27 77 69 64 74 68 3d 64 65 76 69 63 65 2d 77 69 64 74 68 2c 69
6e 69 74 69 61 6c 2d 73 63 61 6c 65 3d 31 27 3e 3c 74 69 74 6c 65 3e 6f
73 6c 61 62 20 2f 20 49 6e 64 65 70 65 6e 64 65 6e 74 20 63 6f 6d 70 75
74 69 6e 67 3c 2f 74 69 74 6c 65 3e 3c 73 74 79 6c 65 3e 62 6f 64 79 7b
6d 61 72 67 69 6e 3a 30 20 61 75 74 6f 3b 70 61 64 64 69 6e 67 3a 32 38
70 78 3b 6d 61 78 2d 77 69 64 74 68 3a 39 30 30 70 78 3b 62 61 63 6b 67
72 6f 75 6e 64 3a 23 66 36 66 33 65 63 3b 63 6f 6c 6f 72 3a 23 32 35 33
31 32 38 3b 66 6f 6e 74 3a 31 36 70 78 2f 31 2e 36 20 73 79 73 74 65 6d
2d 75 69 7d 68 65 61 64 65 72 2c 66 6f 6f 74 65 72 7b 64 69 73 70 6c 61
79 3a 66 6c 65 78 3b 6a 75 73 74 69 66 79 2d 63 6f 6e 74 65 6e 74 3a 73
70 61 63 65 2d 62 65 74 77 65 65 6e 7d 73 6d 61 6c 6c 2c 66 6f 6f 74 65
72 7b 66 6f 6e 74 3a 31 32 70 78 2f 31 2e 36 20 6d 6f 6e 6f 73 70 61 63
65 3b 63 6f 6c 6f 72 3a 23 35 32 36 36 35 37 7d 61 7b 63 6f 6c 6f 72 3a
69 6e 68 65 72 69 74 3b 74 65 78 74 2d 75 6e 64 65 72 6c 69 6e 65 2d 6f
66 66 73 65 74 3a 35 70 78 7d 61 3a 66 6f 63 75 73 2d 76 69 73 69 62 6c
65 7b 6f 75 74 6c 69 6e 65 3a 32 70 78 20 73 6f 6c 69 64 7d 68 31 7b 66
6f 6e 74 3a 63 6c 61 6d 70 28 34 38 70 78 2c 38 76 77 2c 38 38 70 78 29
2f 31 2e 30 35 20 47 65 6f 72 67 69 61 2c 73 65 72 69 66 3b 6c 65 74 74
65 72 2d 73 70 61 63 69 6e 67 3a 2d 33 70 78 3b 6d 61 72 67 69 6e 3a 35
30 70 78 20 30 20 32 34 70 78 7d 70 7b 6d 61 78 2d 77 69 64 74 68 3a 34
34 30 70 78 7d 73 65 63 74 69 6f 6e 7b 62 6f 72 64 65 72 2d 62 6c 6f 63
6b 3a 31 70 78 20 73 6f 6c 69 64 20 23 62 63 63 35 62 38 3b 6d 61 72 67
69 6e 3a 34 38 70 78 20 30 20 32 34 70 78 3b 70 61 64 64 69 6e 67 3a 32
30 70 78 20 30 3b 64 69 73 70 6c 61 79 3a 66 6c 65 78 3b 67 61 70 3a 32
34 70 78 3b 66 6c 65 78 2d 77 72 61 70 3a 77 72 61 70 7d 73 65 63 74 69
6f 6e 20 70 7b 66 6c 65 78 3a 31 3b 6d 69 6e 2d 77 69 64 74 68 3a 31 38
30 70 78 3b 6d 61 72 67 69 6e 3a 30 7d 62 7b 64 69 73 70 6c 61 79 3a 62
6c 6f 63 6b 3b 66 6f 6e 74 2d 73 69 7a 65 3a 31 38 70 78 7d 66 6f 6f 74
65 72 7b 67 61 70 3a 31 36 70 78 3b 66 6c 65 78 2d 77 72 61 70 3a 77 72
61 70 7d 3c 2f 73 74 79 6c 65 3e 3c 68 65 61 64 65 72 3e 3c 73 74 72 6f
6e 67 3e 6f 73 6c 61 62 2e 3c 2f 73 74 72 6f 6e 67 3e 3c 61 20 68 72 65
66 3d 23 69 6e 73 69 64 65 3e 55 6e 64 65 72 20 74 68 65 20 68 6f 6f 64
3c 2f 61 3e 3c 2f 68 65 61 64 65 72 3e 3c 6d 61 69 6e 3e 3c 68 31 3e 53
6d 61 6c 6c 20 62 79 20 64 65 73 69 67 6e 2e 3c 62 72 3e 42 75 69 6c 74
20 66 72 6f 6d 20 74 68 65 20 6d 65 74 61 6c 2e 3c 2f 68 31 3e 3c 70 3e
41 6e 20 69 6e 64 65 70 65 6e 64 65 6e 74 20 6f 70 65 72 61 74 69 6e 67
20 73 79 73 74 65 6d 2e 20 54 68 69 73 20 70 61 67 65 20 74 72 61 76 65
6c 73 20 74 68 72 6f 75 67 68 20 61 20 6b 65 72 6e 65 6c 2c 20 6e 65 74
77 6f 72 6b 20 73 74 61 63 6b 20 61 6e 64 20 64 72 69 76 65 72 20 77 65
20 77 72 6f 74 65 20 6f 75 72 73 65 6c 76 65 73 2e 3c 73 65 63 74 69 6f
6e 20 69 64 3d 69 6e 73 69 64 65 3e 3c 70 3e 3c 73 6d 61 6c 6c 3e 30 31
20 2f 20 46 4f 55 4e 44 41 54 49 4f 4e 3c 2f 73 6d 61 6c 6c 3e 3c 62 3e
4f 75 72 20 6f 77 6e 20 63 6f 64 65 2e 3c 2f 62 3e 42 6f 6f 74 20 74 6f
20 62 72 6f 77 73 65 72 2c 20 6e 6f 20 67 75 65 73 74 20 6c 69 62 72 61
72 69 65 73 2e 3c 70 3e 3c 73 6d 61 6c 6c 3e 30 32 20 2f 20 44 45 4c 49
56 45 52 59 3c 2f 73 6d 61 6c 6c 3e 3c 62 3e 4a 75 73 74 20 74 68 65 20
65 73 73 65 6e 74 69 61 6c 73 2e 3c 2f 62 3e 4f 6e 65 20 70 61 67 65 2e
20 4e 6f 20 73 63 72 69 70 74 73 20 6f 72 20 64 6f 77 6e 6c 6f 61 64 73
2e 3c 2f 73 65 63 74 69 6f 6e 3e 3c 2f 6d 61 69 6e 3e 3c 66 6f 6f 74 65
72 3e 3c 73 70 61 6e 3e 42 41 52 45 20 4d 45 54 41 4c 20 2f 20 4f 50 45
4e 20 53 4f 55 52 43 45 3c 2f 73 70 61 6e 3e 3c 61 20 68 72 65 66 3d 68
74 74 70 73 3a 2f 2f 67 69 74 68 75 62 2e 63 6f 6d 2f 74 61 6b 61 36 37
34 35 2f 6f 73 6c 61 62 3e 52 65 61 64 20 74 68 65 20 73 6f 75 72 63 65
3c 2f 61 3e 3c 2f 66 6f 6f 74 65 72 3e 3c 2f 68 74 6d 6c 3e

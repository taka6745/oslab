; OS-specific regression for authored raw rel8 build checks. No guest fixtures.
; CASE=0 accepts exact signed boundaries and scoped forward/backward labels.
; CASE=1 rejects +128; CASE=2 rejects -129; CASE=3 rejects +142 regression.
%ifndef CASE
%define CASE 0
%endif
bits 64
%include "kernel/machine/branch.inc"
section .text
%if CASE = 0
forward:
    MACHINE_REL8 0xeb,.target
    times 127 db 0x90
.target:
backward:
.target:
    times 126 db 0x90
    MACHINE_REL8 0xeb,.target
second_scope:
    MACHINE_REL8 0x74,.target
    db 0x90
.target:
%elif CASE = 1 || CASE = 3
forward:
    MACHINE_REL8 0xeb,.target
%if CASE = 1
    times 128 db 0x90
%else
    times 142 db 0x90
%endif
.target:
%elif CASE = 2
backward:
.target:
    times 127 db 0x90
    MACHINE_REL8 0xeb,.target
%else
%error unsupported branch regression CASE
%endif
MACHINE_BRANCH_FINISH
section .note.GNU-stack noalloc noexec nowrite progbits

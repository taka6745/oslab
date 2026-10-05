; Project-authored compile-time signed branch checks, NASM 3.02 expression ABI.
; ISA source: Intel SDM Vol. 2. NASM TIMES/macros are build infrastructure.
; Include once per assembly input. Replace raw rel8 branches with MACHINE_REL8
; opcode,target; invoke MACHINE_BRANCH_FINISH after every target is defined.
; Valid checks emit zero bytes. Out-of-range signed offsets fail assembly.
%ifndef OSLAB_MACHINE_BRANCH_INCLUDED
%define OSLAB_MACHINE_BRANCH_INCLUDED 1
%assign MACHINE_BRANCH_COUNT 0
%macro MACHINE_REL8 2
    ; EQU resolves local target scope here, not at the deferred check location.
    %%delta equ %2-($+2)
    db %1,%%delta
%if MACHINE_BRANCH_COUNT = 0
    %xdefine MACHINE_BRANCH_DELTAS %%delta
%else
    %xdefine MACHINE_BRANCH_DELTAS MACHINE_BRANCH_DELTAS,%%delta
%endif
%assign MACHINE_BRANCH_COUNT MACHINE_BRANCH_COUNT+1
%endmacro
%macro MACHINE_BRANCH_ASSERT_LIST 1-*
%rep %0
    times -((%1 < -128) || (%1 > 127)) db 0
%rotate 1
%endrep
%endmacro
%macro MACHINE_BRANCH_FINISH 0
%if MACHINE_BRANCH_COUNT > 0
    MACHINE_BRANCH_ASSERT_LIST MACHINE_BRANCH_DELTAS
%endif
%endmacro
%endif

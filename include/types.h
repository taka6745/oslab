#ifndef OSLAB_TYPES_H
#define OSLAB_TYPES_H
// Compiler-provided ABI constants are intrinsic types, not imported headers/code.
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __SIZE_TYPE__ size_t;
#define SIZE_MAX __SIZE_MAX__
#define NULL ((void *)0)
#define bool _Bool
#define true 1
#define false 0
_Static_assert(sizeof(uint8_t) == 1 && sizeof(uint16_t) == 2 &&
                   sizeof(uint32_t) == 4 && sizeof(uint64_t) == 8,
               "unsupported integer ABI");
#endif

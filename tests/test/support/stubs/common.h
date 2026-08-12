/**
 * @file common.h
 * @brief Host-native stub for common.h used in unit tests.
 *
 * Shadows the real common/common.h (which includes HW-specific sys.h and os.h)
 * so that SDK source files can be compiled with a host toolchain (gcc/clang)
 * without any hardware dependencies.
 */

#ifndef COMMON_H
#define COMMON_H

#include "type.h"   /* real type.h — portable, only uses stdint.h/stdbool.h/stddef.h */

#include <string.h>
#include <assert.h>

/* -------------------------------------------------------------------------
 * ARCH macros — the real arch.h uses RISC-V inline asm.
 * On x86/amd64 we use the same GCC extension ("+r" is arch-independent).
 * ---------------------------------------------------------------------- */

#define ARCH_OPAQUE(x)  do { __asm__ volatile("" : "+r"(x) : : "memory"); } while(0)

/* ARCH_CALL_RA — not needed in host tests; stub to nothing. */
#define ARCH_CALL_RA(name)

/* -------------------------------------------------------------------------
 * OS assertion macros — mapped to standard assert() for host builds.
 * In production firmware these call os_alarm() which locks the CPU in a
 * safe error state. In unit tests a failing assertion aborts the process,
 * which Unity counts as a test failure.
 * ---------------------------------------------------------------------- */

#define OS_ASSERT(condition)            assert(condition)
#define OS_SANITY(condition)            OS_ASSERT(condition)
#define OS_SANITY_NULL(variable)        OS_ASSERT((variable) != NULL)
#define OS_SANITY_BIT32(variable)       OS_ASSERT((variable) < 32)
#define OS_SANITY_ALIGNED(variable)     OS_ASSERT(((variable) & 0x3) == 0)
#define OS_ASSERT_EQUAL(a, b) \
    do { \
        ARCH_OPAQUE(a); \
        ARCH_OPAQUE(b); \
        OS_ASSERT(a == b); \
    } while(0)

/* Compiler attributes that may appear in headers pulled indirectly. */
#define __ISR
#define __FALLTHROUGH   __attribute__((fallthrough))
#define __WEAK          __attribute__((weak))
#define __PACK          __attribute__((__packed__))
#define __ALIGN_U32     __attribute__((aligned(sizeof(u32))))

#endif /* COMMON_H */

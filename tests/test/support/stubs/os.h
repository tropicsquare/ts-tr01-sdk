/**
 * @file os.h
 * @brief Host-native stub for os.h used in unit tests.
 *
 * Shadows the real hal/os.h (which includes arch.h with RISC-V asm) so that
 * SDK source files that #include "os.h" directly (e.g. util.c) can compile
 * on the host. All OS macros are already defined in the common.h stub;
 * this file just prevents the real os.h from being pulled in.
 */

#ifndef OS_H
#define OS_H

#include "common.h"

#endif /* OS_H */

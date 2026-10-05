/**
 * @file os.h
 * @brief Mockable stub for os.h used in unit tests.
 *
 * Shadows the real hal/os.h (which includes arch.h with RISC-V asm) so that SDK
 * source files that #include "os.h" can compile on the host. The OS macros come
 * from the common.h stub; the prototypes below let CMock intercept the OS calls
 * the drivers under test make.
 */

#ifndef OS_H
#define OS_H

#include "common.h"

typedef ts_bool (*os_wait_for_pfunc_t)(void);

ts_bool os_wait_for(os_wait_for_pfunc_t condition, u32 timeout_us);
void os_wait_for_critical(os_wait_for_pfunc_t condition, u32 timeout_us);

void os_delay(u32 ms);
void os_delay_us(u32 us);
void os_delay_cycles(u32 cycles);

void os_sleep(void);
void os_alarm(void);

#endif /* OS_H */

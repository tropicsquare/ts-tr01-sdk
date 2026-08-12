/**
 * @file os.h
 * @author Tropic Square
 * @brief basic HAL interface header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef OS_H
#define OS_H

#include "type.h"
#include "xprintf.h"

#define OS_PRINTF( ... ) xprintf(__VA_ARGS__)

void os_init(void);
typedef u32 os_timer_t;
void os_timer_init(void);

extern u32 timer1_get_time(void);
#define os_timer_get_time timer1_get_time

#define OS_TIMER() os_timer_get_time()

void os_flush(void);
void os_print_msg(ascii *text);
#define OS_PRINT_MSG(text) os_print_msg(text)
#define OS_FLUSH() os_flush()

#define OS_TIMER_MS (1)
#define __ISR __attribute__((interrupt))

/** @name Common shortcuts */
///@{
#define __FALLTHROUGH __attribute__((fallthrough))
#define __WEAK __attribute__((weak))
#define __PACK __attribute__((__packed__))
#define __ALIGN_U32 __attribute__((aligned(sizeof(u32))))
///@}


/**
 * @brief Triggers a system-level alarm if the given condition is false.
 *
 * This macro evaluates a condition and calls @c os_alarm() if the condition fails.
 * Typically used for critical assertion checks in the OS layer.
 *
 * @param condition Expression to evaluate. If false, @c os_alarm() is called.
 */
#define OS_ASSERT(condition) { if (! (condition)) os_alarm(); }

#if (OS_SANITY_DISABLE == 1)
  // we may disable some sanity checks in case lack of space for code
  #define OS_SANITY(condition)
#else // OS_SANITY_DISABLE == 1
  #define OS_SANITY(condition) OS_ASSERT(condition)
#endif // OS_SANITY_DISABLE != 1

/**
 * @brief Asserts that a given pointer is not NULL.
 *
 * This macro checks that the specified variable is not NULL and triggers @c OS_ASSERT
 * if it is. Used as a sanity check for pointer arguments.
 *
 * @param variable Pointer to validate.
 */
#define OS_SANITY_NULL(variable)  OS_SANITY((variable) != NULL)

/**
 * @brief Asserts that a variable represents a valid 5-bit value (less than 32).
 *
 * This macro is used to verify that a variable is a valid index or bit position 
 * within a 32-bit register or mask. Triggers @c OS_ASSERT if the value is 32 or more.
 *
 * @param variable Integer value to validate.
 */
#define OS_SANITY_BIT32(variable) OS_SANITY((variable) < 32)

/**
 * @brief Asserts that a variable is aligned to u32 address.
 *
 * @param variable Integer value to validate.
 */
#define OS_SANITY_ALIGNED(variable)  OS_SANITY(((variable) & 0x3) == 0)

/**
 * @brief Delay using os_sleep() for delay above 1ms.
 *
 * @param[in] ms Time in [ms].
 */
void os_delay(u32 ms);

/**
 * @brief Busy loop delay for times bellow 1ms.
 *
 * @param[in] us Time in [us].
 */
void os_delay_us(u32 us);

/**
 * @brief Busy loop delay number of cycles.
 *
 * @param[in] cycles Number of "nop" in loop.
 */
void os_delay_cycles(u32 cycles);

/**
 * @brief Put CPU to WFI mode.
 */
void os_sleep(void);

/**
 * @brief Enter sleep mode, wait for interrupt from SS.
 */
void os_sleep_mode(void);

/**
 * @brief Minimize power and eneter deep sleep.
 */
void os_power_off(void);

typedef ts_bool (*os_wait_for_pfunc_t)(void);

/**
 * @brief Wait for some condition to be TS_TRUE.
 *
 * If condition is not met in the set timeout it will return TS_FALSE.
 *
 * @param[in] condition Pointer to 'os_wait_for_pfunc_t' type function.
 * @param[in] timeout_us Timeout for condition to be met.
 */
ts_bool os_wait_for(os_wait_for_pfunc_t condition, u32 timeout_us);

/**
 * @brief Wait for some condition to be TS_TRUE.
 *
 * Similar to os_wait_for() but if condition is not met in the set timeout it 
 * cause os_alarm() with dead loop.
 *
 * @param[in] condition Pointer to 'os_wait_for_pfunc_t' type function.
 * @param[in] timeout_us Timeout for condition to be met.
 */
void os_wait_for_critical(os_wait_for_pfunc_t condition, u32 timeout_us);

/**
 * @brief Instant switch to "alarm mode".
 */
void os_alarm(void);

/**
 * @brief Switch to "alarm mode" from ISR.
 * 
 * When critical error happen in ISR we dont want get stuck in ISR endless loop.
 * Such error should be handled in main looop by some flag.
 */
void os_alarm_isr(void);

/**
 * @brief Instant HW reset.
 */
void os_reset(void);

#endif // ! OS_H


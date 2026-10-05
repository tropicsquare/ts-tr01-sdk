/**
 * @file timer.h
 * @author Tropic Square s.r.o.
 * @brief Timer HW driver header file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */
#ifndef TIMER_H
#define TIMER_H

#include "type.h"
#include "timer_regs.h"
#include "cpuss_defs.h"
#include "io_ops.h"

#define TIMER_OVERFLOW_INTERRUPT   (TIMER_INT_EN_OVRFL_INT_EN_MASK)
#define TIMER_THRESHOLD_INTERRUPT  (TIMER_INT_EN_TIME_CMP_INT_EN_MASK)
#define TIMER_IRQ_CHANNEL          (CSR_MIE_MTIE)

/**
 * @brief Timer counting modes.
 */
typedef enum {
    TIMER_CONTINUOUS_MODE = TIMER_CONFIG_THRST_MASK,
    TIMER_SINGLE_SHOT_MODE = 0x0
} timer_count_mode_e;

typedef enum {
    TIMER_1 = TIMER_1_BASE_ADDRESS,
    TIMER_2 = TIMER_2_BASE_ADDRESS
} timer_address_t;

/**
 * Enable timer.
 *
 * @param[in] timer: target timer
 */
void timer_enable(timer_address_t timer);

/**
 * Disable timer.
 *
 * @param[in] timer: target timer
 */
void timer_disable(timer_address_t timer);

static inline void timer_reset(timer_address_t timer)
{
    PTR32_T(timer + TIMER_M_TIME_ADDR) = 0x0;
}

static inline TS_CHECK_RETVAL u32 timer_get_value(timer_address_t timer)
{
    return (PTR32_T(timer + TIMER_M_TIME_ADDR));
}

/**
 * Configure timer.
 *
 * @param[in] timer:     target timer
 * @param[in] prescaler: Clock prescaler select (0..15). See enum TIMER_REGS
 * @param[in] mode:      0 = single-shot mode, 1 = continuous mode.
 * @param[in] threshold: Threshold value to trigger interrupt
 */
void timer_setup(timer_address_t timer, u16 prescaler, timer_count_mode_e mode, u32 threshold);

/**
 * Get timer mode.
 *
 * @param[in] timer: target timer
 */
timer_count_mode_e timer_get_mode(timer_address_t timer) TS_CHECK_RETVAL;

/**
 * Enable timer interrupts.
 *
 * @param[in] timer: target timer
 * @param[in] interrupt_mask: mask with interrupts to enable
 */
void timer_irq_enable(timer_address_t timer, u32 interrupt_mask);

/**
 * Disable timer interrupts.
 *
 * @param[in] timer: target timer
 * @returns mask with previously enabled interrupts
 */
u32 timer_irq_disable(timer_address_t timer) TS_CHECK_RETVAL;

#endif // TIMER_H

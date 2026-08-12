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

/**
 * @brief Timer counting modes.
 */
typedef enum {
    TIMER_CONTINUOUS_MODE = TIMER_CONFIG_THRST_MASK,
    TIMER_SINGLE_SHOT_MODE = 0x0
} timer_count_mode_e;

typedef enum {
    TIMER_OVERFLOW_INTERRUPT = TIMER_INT_EN_OVRFL_INT_EN_MASK,
    TIMER_THRESHOLD_INTERRUPT = TIMER_INT_EN_TIME_CMP_INT_EN_MASK
} timer_interrupt_mode_t;

typedef enum {
    TIMER_1 = TIMER_1_BASE_ADDRESS,
    TIMER_2 = TIMER_2_BASE_ADDRESS
} timer_address_t;

#define TIMER_IRQ_CHANNEL (CSR_MIE_MTIE)

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

static inline u32 timer_get_value(timer_address_t timer)
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
timer_count_mode_e timer_get_mode(timer_address_t timer);

/**
 * Enable timer interrupts.
 *
 * @param[in] timer: target timer
 * @param[in] interrupt: type of interrupt to enable
 */
void timer_irq_enable(timer_address_t timer, timer_interrupt_mode_t interrupt);

/**
 * Disable timer interrupts.
 *
 * @param[in] timer: target timer
 * @returns previously enabled interrupts (timer_interrupt_mode_t)
 */
timer_interrupt_mode_t timer_irq_disable(timer_address_t timer);

#endif // TIMER_H

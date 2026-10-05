/**
 * @file timer.c
 * @author Tropic Square s.r.o.
 * @brief Timer HW driver source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "timer.h"

#include "cpu.h"
#include "irq_ctrl.h"
#include "io_ops.h"
#include "os.h"


// following macro takes 172B of code memory (may be disabled in case out of memory)
#define _SANITY_TIMER(timer)  OS_SANITY(((timer) == TIMER_1) || ((timer) == TIMER_2))
#define _SANITY_TIMER_IRQ_MASK(mask) \
    OS_SANITY(((mask) & ~(TIMER_OVERFLOW_INTERRUPT | TIMER_THRESHOLD_INTERRUPT)) == 0)

static volatile u32 _timer1_cnt = 0;

void timer_enable(timer_address_t timer)
{
    _SANITY_TIMER(timer);
    PTR32_T(timer + TIMER_CONFIG_ADDR) |= TIMER_CONFIG_EN_MASK;
}

void timer_disable(timer_address_t timer)
{
    _SANITY_TIMER(timer);
    PTR32_T(timer + TIMER_CONFIG_ADDR) &= ~TIMER_CONFIG_EN_MASK;
}

void timer_setup(timer_address_t timer, u16 prescaler, timer_count_mode_e mode, u32 threshold)
{
    _SANITY_TIMER(timer);
    PTR32_T(timer + TIMER_CONFIG_ADDR) = (PTR32_T(timer + TIMER_CONFIG_ADDR) & ~TIMER_CONFIG_THRST_MASK) | (mode & TIMER_CONFIG_THRST_MASK);
    PTR32_T(timer + TIMER_M_TIME_CMP_ADDR) = threshold;
    PTR32_T(timer + TIMER_PRESCALER_ADDR) = prescaler;
}

timer_count_mode_e timer_get_mode(timer_address_t timer)
{
    _SANITY_TIMER(timer);
    return PTR32_T(timer + TIMER_CONFIG_ADDR) & TIMER_CONFIG_THRST_MASK;
}

void timer_irq_enable(timer_address_t timer, u32 interrupt_mask)
{
    _SANITY_TIMER(timer);
    _SANITY_TIMER_IRQ_MASK(interrupt_mask);
    PTR32_T(timer + TIMER_INT_EN_ADDR) |= interrupt_mask;
}

u32 timer_irq_disable(timer_address_t timer)
{
    _SANITY_TIMER(timer);
    u32 previous_value = PTR32_T(timer + TIMER_INT_EN_ADDR);

    PTR32_T(timer + TIMER_INT_EN_ADDR) = 0x0;
    return previous_value;
}

static inline void _timer_acknowledge_irq(timer_address_t timer)
{
    _SANITY_TIMER(timer);
    PTR32_T(timer + TIMER_STATUS_ADDR) &= TIMER_STATUS_OVRFL_INT_MASK | TIMER_INT_EN_TIME_CMP_INT_EN_MASK;
}

u32 timer1_get_time(void)
{
    return (_timer1_cnt);
}

__ISR void irq_timer_handler(void)
{
    timer_address_t irq_timer = TIMER_1;

    if ((PTR32_T(TIMER_1_BASE_ADDRESS + TIMER_STATUS_ADDR)) & (PTR32_T(TIMER_1_BASE_ADDRESS + TIMER_INT_EN_ADDR)))
    {
        irq_timer = TIMER_1;
        _timer1_cnt++;

    }
    else if ((PTR32_T(TIMER_2_BASE_ADDRESS + TIMER_STATUS_ADDR)) & (PTR32_T(TIMER_2_BASE_ADDRESS + TIMER_INT_EN_ADDR)))
    {
        irq_timer = TIMER_2;
    }
    else
    {   // should not happen
        os_alarm_isr();
        return;
    }
    _timer_acknowledge_irq(irq_timer);
}

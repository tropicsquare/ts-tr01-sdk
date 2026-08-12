/**
 * @file os.c
 * @author Tropic Square
 * @brief basic HAL interface
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "os.h"

#include "sys.h"
#include "timer.h"
#include "irq_ctrl.h"
#include "io_ops.h"
#include "debug.h"
#include "soc_ctrl.h"
#include "hw.h"
#include "log.h"

LOG_DEF("OS");
#define _OS_ERR_TIMEOUT 1
#define _OS_ERR_ALARM 2
#define _OS_ERR_ALARM_ISR 3


#define _TICKS_TO_MS (1000) // currently counter counts in MHz

#define _US_TO_TICKS(us) (us) // time per count = 1us (prescaler == HW_CLOCK_MHZ)
// NOTE: there should be "_US_TO_TICKS(us) ((us) * _TICKS_TO_MS / 1000) in case  _TICKS_TO_MS != 1000

void os_init(void)
{
    sys_init();
    os_timer_init();
}

void os_timer_init(void)
{
    // enable interrupts on cpu side
    cpu_enable_interrupt(TIMER_IRQ_CHANNEL);

    // setup TIMER_1 as free-run timer for system real time purposes
    timer_setup(TIMER_1, HW_CLOCK_MHZ, TIMER_CONTINUOUS_MODE, _TICKS_TO_MS-1);
    
    // enable interrupts on timer side
    timer_irq_enable(TIMER_1, TIMER_OVERFLOW_INTERRUPT | TIMER_THRESHOLD_INTERRUPT);

    timer_enable(TIMER_1);

    // setup TIMER_2 as single shot timer for universal purposes
    timer_disable(TIMER_2);
    timer_setup(TIMER_2, HW_CLOCK_MHZ, TIMER_SINGLE_SHOT_MODE, UINT32_MAX);
    timer_reset(TIMER_2);
    // ready for use, by enabling
}

void os_delay(u32 ms)
{
    u32 start = os_timer_get_time();

    while ((os_timer_get_time() - start) < ms)
    {
        os_sleep();
    }
}

/// \note does not count more than one ms
void os_delay_us(u32 us)
{
    u32 t;
    
    timer_reset(TIMER_2);
    timer_enable(TIMER_2);  // start timer first, then compute delay for minimal time inaccuracy

    if (us>1000)
    {
        us = 1000; // max 1ms for this type of delay, for longer times use os_delay(ms)
    }

    // convert [us] to timer ticks
    t = _US_TO_TICKS(us);

    while (timer_get_value(TIMER_2) < t)
        ;

    timer_disable(TIMER_2);

    // reset counter value
    timer_reset(TIMER_2);
}

void os_delay_cycles(u32 cycles)
{
    while (cycles--)
    {
        ARCH_NOP(); // to avoid optimization 
        // NOTE: 1ms corresponds to approx 6750 cycles in TROPIC01
    }
}


void os_flush(void)
{
    debug_flush();
}

void os_print_msg(ascii *text)
{
    OS_SANITY_NULL(text);

    while (*text != '\0')
    {
        debug_put_char(*text);
        if (*text == '\n')
        {
            os_flush();
        }
        text++;
    }
}

void os_sleep(void)
{   // suspend to next interrupt
    sys_cpu_sleep();
}

void os_sleep_mode(void)
{
    soc_ctrl_sleep_mode();
}

void os_power_off(void)
{
    soc_ctrl_deep_sleep();
}


ts_bool os_wait_for(os_wait_for_pfunc_t condition, u32 timeout_us)
{
    const u32 timeout = _US_TO_TICKS(timeout_us); 
    ts_bool result = TS_TRUE;

    timer_enable(TIMER_2);

    while (condition() != TS_TRUE)
    {
        if (timer_get_value(TIMER_2) > timeout)
        {
            result = TS_FALSE;
            LOG_ERROR_NUM(_OS_ERR_TIMEOUT);
            LOG_DEBUG("cond: %x", condition);
            break;
        }
        if (timeout_us > 1000)
        {   // in case longer timeout we can use sleep which can take up to 1ms
            sys_cpu_sleep();
        }
    }
    timer_disable(TIMER_2);
    timer_reset(TIMER_2);
    return (result);
}

void os_wait_for_critical(os_wait_for_pfunc_t condition, u32 timeout_us)
{
    if (os_wait_for(condition, timeout_us) == TS_TRUE)
    {
        return;
    }
    os_alarm(); 
    // may continue here only in case os_alarm() not implemented as dead-loop
    while (1)
        ;
}

void os_reset(void)
{
    PTR32_T(SOCCTRL_REG_MAP_BASE_ADDR + SOC_CTRL_CLK_CFG_ADDR) |= SOC_CTRL_CLK_CFG_GRST_MASK;
    // NOTE: pay attention to not switch clock off
}

__attribute__((noreturn)) __WEAK void os_alarm(void)
{
    LOG_ERROR_NUM(_OS_ERR_ALARM);
    // Default stub halts after logging; apps should override (must remain
    // noreturn to honor the contract declared in os.h).
    while (1) {}
}

__WEAK void os_alarm_isr(void)
{
    LOG_ERROR_NUM(_OS_ERR_ALARM_ISR);
    // NOTE: This is dummy function. Should be re-implemented in main app.
}


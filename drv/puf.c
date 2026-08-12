/**
 * @file puf.c
 * @author Tropic Square
 * @brief PUF HW driver.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "puf.h"
#include "os.h"

#include "tassic_defs.h"
#include "io_ops.h"
#include "soc_ctrl.h"
#include "irq_ctrl.h"

#include "puf_regs.h"
#include <string.h>

#define _PUF_REG_WRITE(offset,value) IO_WRITE_32(BDRK_PUF_BASE_ADDR+(offset), value)
#define _PUF_REG_READ(offset)        IO_READ_32(BDRK_PUF_BASE_ADDR+(offset))
#define _PUF_REG_PTR(offset)         PTR32_T(BDRK_PUF_BASE_ADDR+(offset))

#define _PUF_DEFAULT_TIMEOUT (10 * 1000) // [us]

static u32 _condition_bits;

static ts_bool _condition_control_bits_unset(void)
{
    return ((_PUF_REG_READ(PUF_CTRL_CONTROL_ADDR) & _condition_bits) ? TS_FALSE : TS_TRUE);
}

static void _puf_exec_control_task(u32 control_bit)
{
    // set requested bit in CONTROL register 
    _PUF_REG_PTR(PUF_CTRL_CONTROL_ADDR) |= control_bit;
    
    // the bit will be cleared by HW when done, wait for it
    _condition_bits = control_bit;
    os_wait_for_critical(_condition_control_bits_unset, _PUF_DEFAULT_TIMEOUT);
}

void puf_init(u32 read_rate)
{
    puf_wakeup();
    _PUF_REG_WRITE(PUF_CTRL_READ_RATE_ADDR,
        (read_rate << PUF_CTRL_READ_RATE_READ_RATE_POS) & PUF_CTRL_READ_RATE_READ_RATE_MASK
    );
}

void puf_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_PUF_SUBSET);

    // NOTE: we dont switch on analog part (ANA_ENA)
    _PUF_REG_WRITE(PUF_CTRL_CONTROL_ADDR,
            PUF_CTRL_CONTROL_CTRL_READ_ITENA_MASK | PUF_CTRL_CONTROL_CTRL_BCH_ITENA_MASK | PUF_CTRL_CONTROL_CTRL_SPG_ITENA_MASK);
   
    // add some delay do be sure analog is working properly
    os_delay_us (500);
    // NOTE: is seems working without this delay but no timing information found in documentation so we keep it
}

void puf_suspend(void)
{
    _PUF_REG_WRITE(PUF_CTRL_CONTROL_ADDR, 0);
    soc_ctrl_clk_dis(SOC_CTRL_CLK_PUF);
}

void puf_set_mask(u32 mask[PUF_MASK_SIZE32])
{
    int i;
    int n = 0;

    OS_SANITY_NULL(mask);
    // Switch on analog part which is needed only during MASK setup
    // NOTE: It is strongly recommended to disable ANA_ENA after read sequence,
    //       to strongly limit aging effects on analog cells.
    _PUF_REG_PTR(PUF_CTRL_CONTROL_ADDR) |= PUF_CTRL_CONTROL_CTRL_ANA_ENA_MASK;

    for (i = 0; i < (PUF_MASK_SIZE32 / 8); i++) 
    {
        // Select cluster
        _PUF_REG_PTR(PUF_CTRL_CONTROL_ADDR) &= ~PUF_CTRL_CONTROL_CTRL_CLUSTER_MASK;
        _PUF_REG_PTR(PUF_CTRL_CONTROL_ADDR) |= (i << PUF_CTRL_CONTROL_CTRL_CLUSTER_POS);

        // Set mask
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK0_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK1_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK2_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK3_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK4_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK5_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK6_ADDR, mask[n++]);
        _PUF_REG_WRITE(PUF_CTRL_ADD_MASK7_ADDR, mask[n++]);

        // Start read sequence
        // Once any read sequence is done Read start bit will be cleared by HW
        _puf_exec_control_task(PUF_CTRL_CONTROL_CTRL_READ_MASK);
    }
    // Once all read sequences are over, PUF_REG_BANK contains raw puf bits (cant read directly)
}

void puf_set_syndrome(u32 syndrome)
{
    _PUF_REG_WRITE(PUF_CTRL_BCH_ADDR, syndrome);

    // Correction start bit will be cleared to 0 by HW
    _puf_exec_control_task(PUF_CTRL_CONTROL_CTRL_BCH_START_MASK);
}

static ts_bool _puf_read_value(u32 out[PUF_DATA_SIZE32], u32 challenge)
{
    OS_SANITY_NULL(out);

    _PUF_REG_WRITE(PUF_CTRL_CHALLENGE_ADDR, challenge);

    // Key diversification operation is launched when bit SPONGENT_START is raised
    // Spongent start bit SPONGENT_START will be cleared to 0 by HW
    _puf_exec_control_task(PUF_CTRL_CONTROL_CTRL_SPONGENT_START_MASK);

    // Key is stored into ADD_MASKx regs
    out[0] = _PUF_REG_READ(PUF_CTRL_ADD_MASK0_ADDR);
    out[1] = _PUF_REG_READ(PUF_CTRL_ADD_MASK1_ADDR);
    out[2] = _PUF_REG_READ(PUF_CTRL_ADD_MASK2_ADDR);
    out[3] = _PUF_REG_READ(PUF_CTRL_ADD_MASK3_ADDR);
    out[4] = _PUF_REG_READ(PUF_CTRL_ADD_MASK4_ADDR);
    out[5] = _PUF_REG_READ(PUF_CTRL_ADD_MASK5_ADDR);
    out[6] = _PUF_REG_READ(PUF_CTRL_ADD_MASK6_ADDR);
    out[7] = _PUF_REG_READ(PUF_CTRL_ADD_MASK7_ADDR);
    return TS_TRUE;
}

ts_bool puf_read_value(u32 out[PUF_DATA_SIZE32], u32 challenge)
{
    ts_bool result;
    OS_SANITY_NULL(out);

    puf_wakeup();
    result = _puf_read_value(out, challenge);
    puf_suspend();
    return result;
}

// Overrides the weak irq handler defined in irq_ctrl.c
__ISR void irq_brock_puf_handler(void) 
{
    // Check which operation raised the interrupt
    if (_PUF_REG_READ(PUF_CTRL_STATUS_ADDR) & PUF_CTRL_STATUS_READ_DONE_MASK) 
    {   // Read done status
        // Clear status register
        _PUF_REG_PTR(PUF_CTRL_STATUS_ADDR) |= PUF_CTRL_STATUS_READ_DONE_MASK;
        // IRQ used to wake-up CPU and PUF_CTRL_CONTROL_CTRL_READ_MASK will be used
    }
    else if (_PUF_REG_READ(PUF_CTRL_STATUS_ADDR) & PUF_CTRL_STATUS_BCH_DONE_MASK) 
    {   // Error correction done status
        // Clear status register
        _PUF_REG_PTR(PUF_CTRL_STATUS_ADDR) |= PUF_CTRL_STATUS_BCH_DONE_MASK;
    }
    else if (_PUF_REG_READ(PUF_CTRL_STATUS_ADDR) & PUF_CTRL_STATUS_SPONGENT_DONE_MASK) 
    {   // Spongent done status
        // Clear status register
        _PUF_REG_PTR(PUF_CTRL_STATUS_ADDR) |= PUF_CTRL_STATUS_SPONGENT_DONE_MASK;
    }
    else
    {   // unhandled IRQ
        // disable cpu level interrupt
        cpu_disable_interrupt(CSR_MIE_FIRQ8E);
        // raise Alarm mode
        os_alarm_isr();
        return;
    }
}


/**
 * @file macandd.c
 * @author Tropic Square
 * @brief MACANDD (MAC and Destroy) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */


#include "common.h"
#include "macandd.h"
#include "soc_ctrl.h"
#include "io_ops.h"
#include "tassic_defs.h"
#include "macandd_regs.h"

#include "log.h"
LOG_DEF("MAD");

#define _MAD_REG_WRITE(offset, value) IO_WRITE_32(TROPIC01_MEMORY_MAP_MACANDD_BASE_ADDR+(offset), value)
#define _MAD_REG_READ(offset) IO_READ_32(TROPIC01_MEMORY_MAP_MACANDD_BASE_ADDR+(offset))

#define _LOG_DEBUG(...) // LOG_DEBUG(__VA_ARGS__)

// max time for MACANDD operation is about 5ms without secure clock
#define _MAD_TIMEOUT_MAX (20 * 1000) // [us]

static ts_bool _condition_op_done(void)
{
    return (_MAD_REG_READ(MACANDD_STATUS_ADDR) & MACANDD_STATUS_DONE_MASK) ? TS_TRUE : TS_FALSE;
}

void macandd_init(void)
{
    // NOTE: no HW init needed, keep it here as placeholder that it is not missing
}

void macandd_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_MADCLKEN_MASK);
}

void macandd_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_MADCLKEN_MASK);
}

ts_bool macandd_exec(void)
{
    // clear all flags
    _MAD_REG_WRITE(MACANDD_STATUS_ADDR, MACANDD_STATUS_DONE_MASK
                | MACANDD_STATUS_FREAD_ERR_MASK | MACANDD_STATUS_FERASE_ERR_MASK
                | MACANDD_STATUS_BIT_FLIP_ERR_MASK | MACANDD_STATUS_FWRITE_ERR_MASK
                | MACANDD_STATUS_FPROGRAM_ERR_MASK | MACANDD_STATUS_FVRFERS_ERR_MASK
                | MACANDD_STATUS_ATOMICITY_ERR_MASK
            );

    _MAD_REG_WRITE(MACANDD_COMMAND_ADDR, MACANDD_COMMAND_START_MASK);

    if (os_wait_for(_condition_op_done, _MAD_TIMEOUT_MAX) != TS_TRUE)
    {
        return TS_FALSE;
    }
    u32 status = _MAD_REG_READ(MACANDD_STATUS_ADDR);
    
    if (status != MACANDD_STATUS_DONE_MASK)
    {
        LOG_ERROR("st: %x", status);
        return TS_FALSE;
    }

    _LOG_DEBUG("done");
    return TS_TRUE;
}

void macandd_reset(void)
{
    macandd_wakeup();
    _MAD_REG_WRITE(MACANDD_COMMAND_ADDR, MACANDD_COMMAND_CLEAR_KEYS_MASK);
    // NOTE: CLEAR_KEYS is not real command, so dont wait to STATUS_DONE
}

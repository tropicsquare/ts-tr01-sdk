/**
 * @file mbist.c
 * @brief Driver for TSMBIST engine for TROPIC01
 * @author Tropic Square
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "io_ops.h"
#include "os.h"

#include "tassic_defs.h"
#include "mbist_regs.h"
#include "soc_ctrl.h"

#include "mbist.h"

#define _MBIST_REG_READ(offset)             IO_READ_32(TROPIC01_MEMORY_MAP_MBIST_BASE_ADDR+(offset))
#define _MBIST_REG_WRITE(offset,value)      IO_WRITE_32(TROPIC01_MEMORY_MAP_MBIST_BASE_ADDR+(offset), value)


static volatile ts_bool command_in_progress;

static inline void _mbist_reset(void)
{
    command_in_progress = TS_FALSE;
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_RST_MASK);
}

void mbist_init(const mbist_config_t *config)
{
    OS_SANITY_NULL(config);

    mbist_wakeup();
    _mbist_reset();

    // Configure
    _MBIST_REG_WRITE(MBIST_RETENTION_1_ADDR, config->retention_1);
    _MBIST_REG_WRITE(MBIST_RETENTION_2_ADDR, config->retention_2);

    u32 pattern = FIELD_PREP(MBIST_PATTERN_A_MASK, config->pattern_a) |
                  FIELD_PREP(MBIST_PATTERN_B_MASK, config->pattern_b);
    _MBIST_REG_WRITE(MBIST_PATTERN_ADDR, pattern);

    _MBIST_REG_WRITE(MBIST_INT_EN_ADDR, MBIST_STATUS_DONE_MASK);

    // TODO: Fault analysis mode support

    // Keep disabled until test executed
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR,  0);
}

void mbist_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK);
}

void mbist_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK);
}

static u32 _mbist_prepare(mbist_chnls_t chnls)
{
    _MBIST_REG_WRITE(MBIST_MEM_SEL_ADDR, chnls);
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_PREPARE_MASK);

    if ((_MBIST_REG_READ(MBIST_TEST_PROGRESS_ADDR) & chnls) != chnls)
    {
        return MBIST_RES_FAIL;
    }
    if (_MBIST_REG_READ(MBIST_TEST_ERROR_ADDR) & chnls)
    {
        return MBIST_RES_FAIL;
    }
    if (_MBIST_REG_READ(MBIST_TEST_RESULT_ADDR) & chnls)
    {
        return MBIST_RES_FAIL;
    }
    u32 sts_reg = _MBIST_REG_READ(MBIST_STATUS_ADDR);
    if (sts_reg & MBIST_STATUS_DONE_MASK)
    {
        return MBIST_RES_FAIL;
    }
    return MBIST_RES_OK;
}

static void _mbist_run(void)
{
    command_in_progress = TS_TRUE;
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_START_MASK);
    do
    {
        os_sleep();
    } while (command_in_progress != TS_FALSE);
}

u32 mbist_prepare_test(mbist_test_type_e test_type, mbist_chnls_t chnls)
{
    u32 cfg_reg = _MBIST_REG_READ(MBIST_CONFIG_ADDR);
    FIELD_SET(cfg_reg, MBIST_CONFIG_TEST_TYPE_MASK, test_type);

    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg);

    return (_mbist_prepare(chnls));
}

u32 mbist_exec_test(mbist_chnls_t chnls)
{
    u32 cfg_reg = _MBIST_REG_READ(MBIST_CONFIG_ADDR);

    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg | MBIST_CONFIG_MBIST_EN_MASK);

    _mbist_run();

    // restore CFG to disabled state
    cfg_reg &= ~MBIST_CONFIG_MBIST_EN_MASK; // ensure EN disabled
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg);

    // return mask of channels on which MBIST test failed
    return (_MBIST_REG_READ(MBIST_TEST_RESULT_ADDR) ^ chnls);
}

void mbist_erase(mbist_chnls_t chnls)
{
    mbist_wakeup();
    _mbist_reset();

    _MBIST_REG_WRITE(MBIST_INT_EN_ADDR, MBIST_STATUS_DONE_MASK);
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, (MBIST_TEST_MEM_CLR << MBIST_CONFIG_TEST_TYPE_POS) |  MBIST_CONFIG_MBIST_EN_MASK);

    _mbist_prepare(chnls);
    _mbist_run();

    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, 0);
    mbist_suspend();
}


__ISR void irq_mbist_handler(void)
{
    u32 sts_reg = _MBIST_REG_READ(MBIST_STATUS_ADDR);
    _MBIST_REG_WRITE(MBIST_STATUS_ADDR, sts_reg);

    if (sts_reg & MBIST_STATUS_DONE_MASK)
    {
        command_in_progress = TS_FALSE;
    }
}

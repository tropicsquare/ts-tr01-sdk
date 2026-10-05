/**
 * @file spect.c
 * @author Tropic Square
 * @brief Spect source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "spect.h"

#include "tassic_defs.h"
#include "io_ops.h"
#include "soc_ctrl.h"
#include "spect_regs.h"
#include "os.h"

#include "log.h"
LOG_DEF("SPECT");

#define _LOG_DEBUG(...) // LOG_DEBUG(__VA_ARGS__)

#define _SPECT_ERR_TIMEOUT      1
#define _SPECT_ERR_IDLE         2
#define _SPECT_ERR_RESULT       3
#define _SPECT_ERR_INIT_TIMEOUT 4

#define _SPECT_REG_WRITE(offset,value) IO_WRITE_32(SPECT_REG_MAP_BASE_ADDR+(offset), value)
#define _SPECT_REG_READ(offset)        IO_READ_32(SPECT_REG_MAP_BASE_ADDR+(offset))

/**
 * @brief Define max timeout for SPECT command.
 * @note The slowest is ECDSA_SIGN (about 7.8M clk), which is about 110 ms + secure clock penalty.
 */
#define _SPECT_COMMAND_TIMEOUT_MAX  (300 * 1000) // [us]

/**
 * @brief Define max timeout for the SPECT to reach the IDLE state after the soft reset.
 * @note The reset itself settles within a few clock cycles, so this is a generous
 *       upper bound. Keep it <= 1000 us, otherwise os_wait_for() switches to
 *       sys_cpu_sleep() polling with up to 1 ms granularity.
 */
#define _SPECT_RESET_TIMEOUT_US_MAX    (1000) // [us]

static volatile ts_bool _spect_done;
static volatile ts_bool _spect_error;

static TS_CHECK_RETVAL ts_bool _condition_spect_idle(void)
{
    return ((_SPECT_REG_READ(SPECT_STATUS_ADDR) & SPECT_STATUS_IDLE_MASK) ? TS_TRUE : TS_FALSE);
}

ts_bool spect_init(void)
{
    _spect_done = TS_FALSE;
    _spect_error = TS_FALSE;
 
    spect_wakeup();
    spect_reset();
    
    // wait for IDLE
    if (os_wait_for(_condition_spect_idle, _SPECT_RESET_TIMEOUT_US_MAX) != TS_TRUE)
    {
        LOG_ERROR_NUM(_SPECT_ERR_INIT_TIMEOUT);
        _LOG_DEBUG("ST: %x", _SPECT_REG_READ(SPECT_STATUS_ADDR));
        return TS_FALSE;
    }

    // enable interrupts
    _SPECT_REG_WRITE(SPECT_INT_ENA_ADDR, SPECT_INT_ENA_INT_DONE_EN_MASK | SPECT_INT_ENA_INT_ERR_EN_MASK);

    // clear SPECT RAM
    for (u32 i=0; i<SPECT_RAM_IN_SIZE; i+=sizeof(u32))
    {
        spect_write_dram_in_u32(i, 0);
    }

    return TS_TRUE;
}

void spect_scramble(void)
{
    // Make SPECT register file scrambling 
    _SPECT_REG_WRITE(SPECT_CONFIG_ADDR, SPECT_CONFIG_RF_SCRAM_EN_MASK | SPECT_CONFIG_RF_SCRAM_INIT_MASK);
    // NOTE: to have it working, the SEC_CNTR_PRECHARGE_SPECT_EN must be enabled
}

void spect_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_SPECTCLKEN_MASK);
}

void spect_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_SPECTCLKEN_MASK);
}

void spect_reset(void)
{
    _SPECT_REG_WRITE(SPECT_COMMAND_ADDR, SPECT_COMMAND_SOFT_RESET_MASK);
}

void spect_exec_cmd(void)
{
    _spect_done = TS_FALSE;
    _spect_error = TS_FALSE;
    _SPECT_REG_WRITE(SPECT_COMMAND_ADDR, SPECT_COMMAND_START_MASK);
}

static void _op_init(spect_op_id_t op_id, size_t size, ts_bool use_cpb)
{
    u32 cfg_word = 0;

    cfg_word |= (op_id << SPECT_CFG_WORD_OP_ID_POS) & SPECT_CFG_WORD_OP_ID_MASK;
    // configure source and destination memory type 
    if (use_cpb == TS_TRUE)
    {
        cfg_word |= (SPECT_INPUT_SRC_CMD_BUFFER << SPECT_CFG_WORD_IN_SRC_POS) | (SPECT_OUTPUT_DST_CMD_BUFFER << SPECT_CFG_WORD_OUT_DST_POS);
    }
    else
    {
        cfg_word |= (SPECT_INPUT_SRC_DATA_IN << SPECT_CFG_WORD_IN_SRC_POS) | (SPECT_OUTPUT_DST_DATA_OUT << SPECT_CFG_WORD_OUT_DST_POS);
    }
    // update size of data in
    cfg_word |= (size << SPECT_CFG_WORD_IN_SIZE_POS) & SPECT_CFG_WORD_IN_SIZE_MASK;

    spect_write_dram_in_u32(SPECT_OFFSET_CFG_WORD, cfg_word);
}

void spect_op_init(spect_op_id_t op_id, size_t size)
{
    _op_init(op_id, size, TS_FALSE);
}

void spect_cpb_op_init(spect_op_id_t op_id, size_t size)
{
    _op_init(op_id, size, TS_TRUE);
}

ts_bool spect_cpb_op_task(spect_op_id_t op_id, size_t size)
{
    _LOG_DEBUG("OP %x", op_id);
    spect_cpb_op_init(op_id, size);
    // run operation using data prepared in CPB buffer
    spect_exec_cmd();
    return spect_wait_done();
}

u32 spect_read_dram_out_u32(u32 offset)
{
    OS_SANITY_ALIGNED(offset);
    return IO_READ_32(SPECT_D_RAM_OUT_BASE_ADDR + offset);
}

void spect_read_dram_out(u8 *dest, u32 offset, size_t len)
{
    sys_copy_regs_to_mem(dest, SPECT_D_RAM_OUT_BASE_ADDR + offset, len);
}

void spect_write_dram_in_u32(u32 offset, u32 value)
{
    IO_WRITE_32(SPECT_D_RAM_IN_BASE_ADDR + offset, value);
}

void spect_write_dram_in(u32 offset, const u8 *data, size_t len)
{
    // NOTE: data == NULL is here valid in case len == 0
    sys_copy_mem_to_regs(SPECT_D_RAM_IN_BASE_ADDR + offset, data, len);
}

void spect_write_fw(u32 offset, const u32 *data, size_t len)
{
    len >>= 2; // bytes to words
    
    OS_SANITY_NULL(data);
    OS_SANITY_ALIGNED(offset);

    while (len--)
    {
        IO_WRITE_32(SPECT_I_MEM_BASE_ADDR + offset, *data);
        data++;
        offset += sizeof(u32);
    }
}

static TS_CHECK_RETVAL ts_bool _condition_spect_done(void)
{
    if (_spect_done == TS_TRUE)
    {
        return TS_TRUE;
    }
    if (_SPECT_REG_READ(SPECT_STATUS_ADDR) & SPECT_STATUS_IDLE_MASK)
    {
        return TS_TRUE;
    }
    return TS_FALSE;
}

ts_bool spect_wait_op_done(void)
{ 
    if (os_wait_for(_condition_spect_done, _SPECT_COMMAND_TIMEOUT_MAX) != TS_TRUE)
    {
        LOG_ERROR_NUM(_SPECT_ERR_TIMEOUT);
        _LOG_DEBUG("ST: %x", _SPECT_REG_READ(SPECT_STATUS_ADDR));
        return TS_FALSE;
    }
    return _spect_done;
}

ts_bool spect_wait_done(void)
{
    if (spect_wait_op_done() != TS_TRUE)
    {
        return TS_FALSE;
    }

    spect_result_code_t result = spect_result_code();

    _LOG_DEBUG("RES: %x", spect_read_dram_out_u32(SPECT_OFFSET_RES_WORD));
    
    if ((result != SPECT_OP_OK) || (_spect_error == TS_TRUE))
    {
        LOG_ERROR_NUM(_SPECT_ERR_RESULT); 
        _spect_error = TS_TRUE;
        return TS_FALSE;
    }

    if (_spect_done != TS_TRUE)
    {
        LOG_ERROR_NUM(_SPECT_ERR_IDLE);
    }
    return _spect_done;
}

spect_result_code_t spect_result_code(void)
{
    return (spect_read_dram_out_u32(SPECT_OFFSET_RES_WORD) & SPECT_RES_WORD_OP_STATUS_MASK);
}

size_t spect_result_size(void)
{
    return ((spect_read_dram_out_u32(SPECT_OFFSET_RES_WORD) \
                & SPECT_RES_WORD_DATA_OUT_SIZE_MASK) >> SPECT_RES_WORD_DATA_OUT_SIZE_POS);
}

u32 *spect_address(void)
{
    return ((u32 *)SPECT_I_MEM_BASE_ADDR);
}

__ISR void irq_spect_handler(void)
{
    u32 reg = _SPECT_REG_READ(SPECT_STATUS_ADDR);

    if (reg & SPECT_STATUS_DONE_MASK)
    {   // DONE has mode W1C
        _SPECT_REG_WRITE(SPECT_STATUS_ADDR, SPECT_STATUS_DONE_MASK);
        _spect_done = TS_TRUE;
    }

    if (reg & SPECT_STATUS_ERR_MASK)
    {   // ERR has mode W1C
        _SPECT_REG_WRITE(SPECT_STATUS_ADDR, SPECT_STATUS_ERR_MASK);
        _spect_error = TS_TRUE;
        _spect_done = TS_TRUE;
    }
}

/*******************************************************
 * API for TASSIC Top verification. Do not remove!
*******************************************************/
ts_bool spect_get_done_flag(void) 
{
    return _spect_done;
}

ts_bool spect_get_error_flag(void)
{
    return _spect_error;
}


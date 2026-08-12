/**
 * @file cpb.c
 * @author Tropic Square
 * @brief CPB (Command Processing Block) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "cpb.h"

#include "soc_ctrl.h"
#include "tassic_defs.h"
#include "cpb_regs.h"
#include "io_ops.h"

#include "log.h"
LOG_DEF("CPB");

#define _CPB_REG_WRITE(offset, value) IO_WRITE_32(TROPIC01_MEMORY_MAP_CPB_BASE_ADDR+(offset), value)
#define _CPB_REG_READ(offset) IO_READ_32(TROPIC01_MEMORY_MAP_CPB_BASE_ADDR+(offset))

#define _LOG_DEBUG(...) // LOG_DEBUG(__VA_ARGS__)


static inline void _copy_regs_to_mem(u8 *dest, u32 addr, size_t size)
{
    sys_copy_regs_to_mem(dest, TROPIC01_MEMORY_MAP_CPB_BASE_ADDR + addr, size);
}

static inline void _copy_mem_to_regs(u32 addr, u8 *src, size_t size)
{
    sys_copy_mem_to_regs(TROPIC01_MEMORY_MAP_CPB_BASE_ADDR + addr, src, size);
}

void cpb_init(u8 error_code)
{
    cpb_wakeup();
    _LOG_DEBUG("init 0x%x",  _CPB_REG_READ(CPB_BLOCK_ID_ADDR));
   
    cpb_set_error_code(error_code);
    
    cpb_result_reset();
    cpb_clear_descriptors();
    cpb_suspend();
}

void cpb_suspend(void)
{
    _CPB_REG_WRITE(CPB_CONFIG_ADDR, 0);
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_CPBCLKEN_MASK);
}

void cpb_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_CPBCLKEN_MASK);
    _CPB_REG_WRITE(CPB_CONFIG_ADDR, CPB_CONFIG_EPCK_MASK);
}

void cpb_set_descriptor(u32 id, u32 value)
{
    OS_ASSERT(id < CPB_NUM_DESCRIPTORS);

    _CPB_REG_WRITE(CPB_OFFSET_DESCRIPTOR_MEM + (id << 2), value);
}

void cpb_clear_descriptors(void)
{
    for (u32 id=0; id<CPB_NUM_DESCRIPTORS; id++)
    {
        _CPB_REG_WRITE(CPB_OFFSET_DESCRIPTOR_MEM + (id << 2), 0);
    }
}

void cpb_command_pointer(size_t offset)
{   // NOTE: here we overwrite result pointer, so we cant use both at same time
    _CPB_REG_WRITE(CPB_POINTERS_ADDR, offset << CPB_POINTERS_CMD_PTR_POS);
}

void cpb_result_pointer(size_t offset)
{
    _CPB_REG_WRITE(CPB_POINTERS_ADDR, offset << CPB_POINTERS_RES_PTR_POS);
}

u8 cpb_get_command(void)
{
    return ((_CPB_REG_READ(CPB_PCR_RES_ADDR) & CPB_PCR_RES_CMD_ID_MASK) >> CPB_PCR_RES_CMD_ID_POS);
}

void cpb_result_reset(void)
{
    // reset all W1C status flags
    _CPB_REG_WRITE(CPB_STATUS_ADDR, CPB_STATUS_PCD_MASK | CPB_STATUS_CNA_MASK | CPB_STATUS_SEC_MASK);
    // reset pointers
    _CPB_REG_WRITE(CPB_POINTERS_ADDR, 0);
}

u8 cpb_result_code(void)
{
#define  _PCR_CMD_AUTH           (0x1 << CPB_STATUS_PCR_POS)
#define  _PCR_CMD_NOT_AUTH       (0x2 << CPB_STATUS_PCR_POS)
#define  _PCR_CMD_INVALID        (0x4 << CPB_STATUS_PCR_POS)
#define  _PCR_CMD_DESCRIPTOR_ERR (0x8 << CPB_STATUS_PCR_POS)

    u32 status = _CPB_REG_READ(CPB_STATUS_ADDR);

    _LOG_DEBUG("ST: %x", status);

    if ((status & CPB_STATUS_PCD_MASK) == 0)
    {
        return (CPB_RESULT_BUSY);
    }
    
    if ((status & _PCR_CMD_INVALID) || (cpb_get_command() == 0))
    {
        _LOG_DEBUG("RES: %x", _CPB_REG_READ(CPB_PCR_RES_ADDR));
        return (CPB_RESULT_INVALID_COMMAND);
    }

    if (status & (CPB_STATUS_CNA_MASK | _PCR_CMD_NOT_AUTH))
    {
        return (CPB_RESULT_AUTH_FAILED);
    }
    
    if (status & _PCR_CMD_DESCRIPTOR_ERR)
    {
        return (CPB_RESULT_DESCRIPTOR_ERR);
    }

    if (status & _PCR_CMD_AUTH)
    {
        return (CPB_RESULT_AUTH_OK);
    }

    return (CPB_RESULT_BUSY);
}

void cpb_set_error_code(u8 value)
{
    _CPB_REG_WRITE(CPB_CODES_ADDR, (value << CPB_CODES_UNAUTHORIZED_POS));
}

ts_bool cpb_read_data(u8 *dest, size_t offset, size_t len)
{
    OS_ASSERT(len+offset <= CPB_COMMAND_BUFFER_SIZE);
    _copy_regs_to_mem(dest, CPB_OFFSET_COMMAND_BUFFER + offset, len);
    return TS_TRUE;
}

void cpb_write_data(u8 *src, size_t offset, size_t len)
{
    OS_ASSERT(len+offset <= CPB_RESULT_BUFFER_SIZE);
    _copy_mem_to_regs(CPB_OFFSET_RESULT_BUFFER + offset, src, len);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt Handler
////////////////////////////////////////////////////////////////////////////////////////////////////

__ISR void irq_cpb_handler(void)
{
    _LOG_DEBUG("CPB Interrupt !");
    os_alarm_isr();
}


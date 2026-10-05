/**
 * @file kdb.c
 * @author Tropic Square
 * @brief KDB (Key Distribution Block) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "cpu.h"
#include "csr.h"
#include "os.h"

#include "kdb.h"

#include "tassic_defs.h"
#include "io_ops.h"
#include "soc_ctrl.h"
#include "kdb_regs.h"
#include "irq_ctrl.h"

#include "log.h"
LOG_DEF("KDB");

#define _KDB_REG_WRITE(offset,value)        IO_WRITE_32(KDB_REG_MAP_BASE_ADDR+(offset), value)
#define _KDB_REG_READ(offset)               IO_READ_32(KDB_REG_MAP_BASE_ADDR+(offset))
#define _KDB_REG_PTR(offset)                PTR32_T(KDB_REG_MAP_BASE_ADDR+(offset))

#define _KDB_MEM_ENTRY_SIZE (0x10)

void (*debug_cb)(u32 *debug_data, u32 *debug_error_flag) = NULL;


void kdb_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_KDBCLKEN_MASK);
}

void kdb_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_KDBCLKEN_MASK);
}

static void _kdb_set_config_mem_entry(const kdb_cfg_mem_entry_t *entry, int entry_index)
{
    u32 entry_addr = (entry_index * _KDB_MEM_ENTRY_SIZE);

    sys_copy_mem_to_regs(KDB_CONFIG_MEM_BASE_ADDR + entry_addr, (u8 *)entry, sizeof(kdb_cfg_mem_entry_t));
}

void kdb_init(const kdb_config_t *cfg, const kdb_cfg_mem_entry_t *cfg_mem, int n_cfg_mem_entries)
{
    OS_ASSERT(n_cfg_mem_entries <= KDB_NUM_CONFIG_MEM_ENTRIES);

    kdb_wakeup();

    // Configure mode
    u32 tmp = _KDB_REG_READ(KDB_CONFIG_ADDR);

    if (cfg->mode != KDB_DONT_SET_MODE)
    {
        FIELD_SET(tmp, KDB_CONFIG_MODE_MASK, cfg->mode);
    }
    FIELD_SET(tmp, KDB_CONFIG_DBGWT_MASK, cfg->debug_wait);

    _KDB_REG_WRITE(KDB_CONFIG_ADDR, tmp);

    // Fill in the configuration memory
    for (int i = 0; i < n_cfg_mem_entries; i++)
    {
        _kdb_set_config_mem_entry(&cfg_mem[i], i);
    }

    // Enable interrupts
    //  - Memory map error, Key Type Error, Deadlock Error always -> Should not happen
    //  - Debug wait -> Only in debug mode
    tmp = BIT(KDB_INT_EN_MEM_MAP_ERR_INT_EN_POS) |
          BIT(KDB_INT_EN_KTYPE_ERR_INT_EN_POS) |
          BIT(KDB_INT_EN_DEADLOCK_INT_EN_POS);

    // Buffer pointer to callback that will fill in the next Debug data upon
    // Debug transfer pendign interrupt!
    if (cfg->mode == KDB_DEBUG_MODE) 
    {
        FIELD_SET(tmp, KDB_INT_EN_DTRPND_INT_EN_MASK, 1);
        if (cfg->debug_wait == TS_TRUE)
        {
            debug_cb = cfg->debug_cb;
        }
    }

    _KDB_REG_WRITE(KDB_INT_EN_ADDR, tmp);
}

__ISR void irq_kdb_handler(void)
{
    u32 status = _KDB_REG_READ(KDB_STATUS_ADDR);

    // Alarm Mode on deadlock or wrong configuration, should never happen!
    if (status & (KDB_STATUS_MEM_MAP_ERR_MASK | KDB_STATUS_KTYPE_ERR_MASK | KDB_STATUS_DEADLOCK_MASK))
    {
        cpu_disable_interrupt(CSR_MIE_FIRQ14E);
        LOG_DEBUG("ST: %x", status);
        os_alarm_isr();
    }

    // In the debug mode:
    //  - Get the next debug key value from callback
    //  - Feed to the HW before clearing the flag to unblock the KBUS transfer!
    if ((status & KDB_STATUS_DTRPND_MASK) && (debug_cb != NULL))
    {
        u32 debug_key = 0;
        u32 debug_error_flag = 0;

        debug_cb(&debug_key, &debug_error_flag);
        _KDB_REG_WRITE(KDB_DEBUG_KEY_ADDR, debug_key);
        _KDB_REG_WRITE(KDB_DEBUG_ERROR_FLAG_ADDR, debug_error_flag);
    }

    // Clear the status
    _KDB_REG_WRITE(KDB_STATUS_ADDR, status);
}

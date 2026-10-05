/**
 * @file edb.c
 * @author Tropic Square
 * @brief EDB (Entropy Distribution Block) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "edb.h"
#include "tassic_defs.h"
#include "io_ops.h"
#include "soc_ctrl.h"
#include "edb_regs.h"

#include "log.h"
LOG_DEF("EDB");

#define _EDB_REG_WRITE(offset,value)        IO_WRITE_32(EDB_REG_MAP_BASE_ADDR+(offset), value)
#define _EDB_REG_READ(offset)               IO_READ_32(EDB_REG_MAP_BASE_ADDR+(offset))


void edb_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_EDBCLKEN_MASK);
}

void edb_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_EDBCLKEN_MASK);
}

void edb_init(void)
{
    edb_wakeup();

    // Read current configuration
    u32 tmp = _EDB_REG_READ(EDB_CONFIG_ADDR);
    
    // Keep MODE as is (verification purposes)
    // the mode is EDB_NORMAL_MODE after POR
    tmp &= EDB_CONFIG_MODE_MASK;

    tmp |= ( ( 0 << EDB_CONFIG_DBGWT_POS)
           | (20 << EDB_CONFIG_KCKRNDS_POS)
           | ( 8 << EDB_CONFIG_TRAWTH_POS)
           | ( 4 << EDB_CONFIG_ENPYRNDS_POS)
           | ( 4 << EDB_CONFIG_DMYRNDS_POS)
    );
    _EDB_REG_WRITE(EDB_CONFIG_ADDR, tmp);
}

void edb_init_cfg(const edb_cfg_t *edb_cfg)
{
    OS_SANITY_NULL(edb_cfg);

    edb_wakeup();

    u32 tmp = _EDB_REG_READ(EDB_CONFIG_ADDR);
    if (edb_cfg->mode != EDB_DONT_SET_MODE)
    {
        FIELD_SET(tmp, EDB_CONFIG_MODE_MASK, edb_cfg->mode);
    }
    FIELD_SET(tmp, EDB_CONFIG_DBGWT_MASK, edb_cfg->debug_wait);
    FIELD_SET(tmp, EDB_CONFIG_KCKRNDS_MASK, edb_cfg->keccak_rounds);
    FIELD_SET(tmp, EDB_CONFIG_TRAWTH_MASK, edb_cfg->raw_threshold);
    FIELD_SET(tmp, EDB_CONFIG_ENPYRNDS_MASK, edb_cfg->absorb_rounds);
    FIELD_SET(tmp, EDB_CONFIG_DMYRNDS_MASK, edb_cfg->dummy_rounds);

    _EDB_REG_WRITE(EDB_CONFIG_ADDR, tmp);
}

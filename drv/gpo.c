/**
 * @file gpo.c
 * @author Tropic Square
 * @brief GPO control driver
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "gpo.h"

#include "io_ops.h"
#include "tassic_defs.h"
#include "soc_ctrl_regs.h"

#define _ADDR(offset) (SOCCTRL_REG_MAP_BASE_ADDR + (offset))
#define _GPO_REG_PTR32(offset) PTR32_T(SOCCTRL_REG_MAP_BASE_ADDR+(offset))

typedef struct {
    const u8 pos;
    const u8 offset;
} gpo_setup_t;

static const gpo_setup_t _GPO[GPO_NUM] = {
    [GPO0] = {SOC_CTRL_GPO_CTRL_1_GPO0_VAL_POS, SOC_CTRL_GPO_CTRL_1_ADDR},
    [GPO1] = {SOC_CTRL_GPO_CTRL_1_GPO1_VAL_POS, SOC_CTRL_GPO_CTRL_1_ADDR},
    [GPO2] = {SOC_CTRL_GPO_CTRL_1_GPO2_VAL_POS, SOC_CTRL_GPO_CTRL_1_ADDR},
    [GPO3] = {SOC_CTRL_GPO_CTRL_1_GPO3_VAL_POS, SOC_CTRL_GPO_CTRL_1_ADDR},
    [GPO4] = {SOC_CTRL_GPO_CTRL_2_GPO4_VAL_POS, SOC_CTRL_GPO_CTRL_2_ADDR},
};

static ts_bool gpo_int_enable;

static u32 _gpo_bit(gpo_e gpo)
{
    if (gpo >= GPO_NUM)
    {
        return (0);
    }
    return (1UL << _GPO[gpo].pos);
}

static u32 _gpo_addr(gpo_e gpo)
{
    if (gpo >= GPO_NUM)
    {
        return (0);
    }
    return (_ADDR(_GPO[gpo].offset));
}

void gpo_init(void)
{
    // NOTE: Dont need to enable periphery clock, because BUSCLKEN enabled by default
    gpo_int_enable = TS_FALSE;
}

void gpo_set_src(gpo_e gpo, u32 src)
{
    u32 addr = _gpo_addr(gpo);
    u32 mask;

    switch (gpo)
    {
    case GPO0:
        src <<= SOC_CTRL_GPO_CTRL_1_GPO0_SRC_POS;
        mask = SOC_CTRL_GPO_CTRL_1_GPO0_SRC_MASK;
        break;

    case GPO1:
        src <<= SOC_CTRL_GPO_CTRL_1_GPO1_SRC_POS;
        mask = SOC_CTRL_GPO_CTRL_1_GPO1_SRC_MASK;
        break;

    case GPO2:
        src <<= SOC_CTRL_GPO_CTRL_1_GPO2_SRC_POS;
        mask = SOC_CTRL_GPO_CTRL_1_GPO2_SRC_MASK;
        break;

    case GPO3:
        src <<= SOC_CTRL_GPO_CTRL_1_GPO3_SRC_POS;
        mask = SOC_CTRL_GPO_CTRL_1_GPO3_SRC_MASK;
        break;

    case GPO4: // GPO4 does not have configurable source
    default:
        return;
    }

    PTR32_T(addr) = (PTR32_T(addr) & ~mask) | (src & mask);
}

void gpo_on(gpo_e gpo)
{
    u32 addr = _gpo_addr(gpo);
    u32 bit  = _gpo_bit(gpo);

    if (bit == 0)
    {
        return;
    }
    PTR32_T(addr) |= bit;
}

void gpo_off(gpo_e gpo)
{
    u32 addr = _gpo_addr(gpo);
    u32 bit  = _gpo_bit(gpo);

    if (bit == 0)
    {
        return;
    }
    PTR32_T(addr) &= ~bit;
}

bool gpo_state(gpo_e gpo)
{
    u32 addr = _gpo_addr(gpo);
    u32 bit  = _gpo_bit(gpo);

    if (bit == 0)
    {
        return(false);
    }
    return ((PTR32_T(addr) & bit) ? true : false);
}

void gpo_toggle(gpo_e gpo)
{
    if (gpo >= GPO_NUM)
    {
        return;
    }
    if (gpo_state(gpo))
    {
        gpo_off(gpo);
    }
    else
    {
        gpo_on(gpo);
    }
}

// The only application available GPO in provisioned state is GPO4.
// GPO4 does not have SRC configuration, so we have much simpler API for it.
static inline void _gpo4_on(void)
{
    _GPO_REG_PTR32(SOC_CTRL_GPO_CTRL_2_ADDR) |= SOC_CTRL_GPO_CTRL_2_GPO4_VAL_MASK;
}

static inline void _gpo4_off(void)
{
    _GPO_REG_PTR32(SOC_CTRL_GPO_CTRL_2_ADDR) &= ~SOC_CTRL_GPO_CTRL_2_GPO4_VAL_MASK;
}

void gpo4_set_mode(u32 mode)
{
    switch (mode)
    {
    case GPO4_MODE_INTERRUPT:
        gpo_int_enable = TS_TRUE;
        break;
    case GPO4_MODE_ALWAYS_HIGH:
        gpo4_on();
        break;
    case GPO4_MODE_ALWAYS_LOW:
        gpo4_off();
        break;
    default:
        break;
    }
}

void gpo4_on(void)
{
    _gpo4_on();
}

void gpo4_off(void)
{
    _gpo4_off();
}

void gpo_int_set(void)
{
    if (gpo_int_enable == TS_TRUE)
    {
       _gpo4_on();    
    }
}

void gpo_int_clr(void)
{
    if (gpo_int_enable == TS_TRUE)
    {
       _gpo4_off();
    }
}

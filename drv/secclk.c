/**
 * @file secclk.c
 * @author Tropic Square
 * @brief Secure Clock (Stealing clocks) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "secclk.h"
#include "tassic_defs.h"
#include "io_ops.h"
#include "sc_regs.h"

#define _SECCLK_REG_WRITE(offset,value) IO_WRITE_32(BDRK_SECCLK_BASE_ADDR+(offset),value)

/* 
Recommended setup :
  First, select the desired interval from which the stealing values should be from and then set RCS_CONFIG and RCS_LUT_CONFIG to listed values.
  If the last interval (24-31 or 28-31) is selected, the value 31 is included, which is the value for no stealing.

Divided into quarters:
 Intensity | interval | RCS_CONFIG[4:0] | RCS_LUT_CONFIG[31:0]
  the most      0-7         0x03            0xCBA90123
        -      8-15         0x0B            0xCBA90123
        -     16-23         0x13            0xCBA90123
 the least    24-31         0x1B            0xCBA90123

Divided into eighths:
 Intensity | interval | RCS_CONFIG[4:0] | RCS_LUT_CONFIG[31:0]
  the most      0-3         0x01            0xA901A901
        -       4-7         0x05            0xA901A901
        -      8-11         0x09            0xA901A901
        -     12-15         0x0D            0xA901A901
        -     16-19         0x11            0xA901A901
        -     20-23         0x15            0xA901A901
        -     24-27         0x19            0xA901A901
 the least    28-31         0x1D            0xA901A901
*/

#define _RCS_CFG_00_03 (0x01)
#define _RCS_CFG_04_07 (0x05)
#define _RCS_CFG_08_11 (0x09)
#define _RCS_CFG_12_15 (0x0D)
#define _RCS_CFG_16_19 (0x11)
#define _RCS_CFG_20_23 (0x15)
#define _RCS_CFG_24_27 (0x19)
#define _RCS_CFG_28_31 (0x1D)

// Random selected nibble from LUT_CONFIG is used as offset to stealing pattern + RCS_CFG
#define _LUT_CONFIG_QUARTERS 0xCBA90123 // 0b 1100 1011 1010 1001 0000 0001 0010 0011 == +4 +3 +2 +1  0 -1 -2 -3
#define _LUT_CONFIG_EIGHTHS  0xA901A901 // 0b 1010 1001 0000 0001 1010 1001 0000 0001 == +2 +1  0 -1 +2 +1  0 -1

enum {
    _CG_CONFIG_JITTER_OFF = 0,
    _CG_CONFIG_JITTER_5   = 1,// 5% (same for value of 2)
    _CG_CONFIG_JITTER_MAX = 3 // 10%
};

void secclk_trim(u32 trim)
{
    u32 tmp = FIELD_PREP(SECURE_CLOCK_CG_CONFIG_JITTER_ENA_MASK, _CG_CONFIG_JITTER_MAX) // add 10 % of jitter
            | FIELD_PREP(SECURE_CLOCK_CG_CONFIG_FM_ENA_MASK, 1)         // enable frequency monitor
            | FIELD_PREP(SECURE_CLOCK_CG_CONFIG_TRIM_CLK_MASK, trim);   // put trim value

    _SECCLK_REG_WRITE(SECURE_CLOCK_CG_CONFIG_ADDR, tmp);
}

void secclk_init(secclk_stealing_intensity_e intensity)
{
    const u8 RCS_CFG[SECCLK_STEALING_INTENSITY_NUM] = {
        _RCS_CFG_28_31, _RCS_CFG_24_27, _RCS_CFG_20_23, _RCS_CFG_16_19, _RCS_CFG_12_15, _RCS_CFG_08_11, _RCS_CFG_04_07, _RCS_CFG_00_03
    };

    OS_ASSERT((intensity >= SECCLK_STEALING_INTENSITY_MIN) && (intensity <= SECCLK_STEALING_INTENSITY_MAX));

    _SECCLK_REG_WRITE(SECURE_CLOCK_RCS_LUT_CONFIG_ADDR, _LUT_CONFIG_EIGHTHS);
    _SECCLK_REG_WRITE(SECURE_CLOCK_RCS_CTRL_ADDR,
            SECURE_CLOCK_RCS_CTRL_RCS_ENA_MASK | (RCS_CFG[intensity] << SECURE_CLOCK_RCS_CTRL_RCS_CONFIG_POS));
}


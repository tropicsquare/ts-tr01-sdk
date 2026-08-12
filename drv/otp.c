/**
 * @file otp.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief OTP controller HW driver source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "otp.h"

#include "hw.h"
#include "io_ops.h"
#include "layout_otp.h"
#include "log.h"
#include "otp_regs.h"
#include "prng.h"
#include "scramble.h"
#include "soc_ctrl.h"
#include "tassic_defs.h"

LOG_DEF("OTP");

enum {
    _OTP_ERR_UNHANDLED_IRQ   = 1,
    _OTP_ERR_STATUS_MISMATCH = 2
};

#define _OTP_REG_READ(offset)         IO_READ_32(OTP_MEMORY_OTP_CONTROLLER_REGISTER_MAP_BASE_ADDR+(offset))
#define _OTP_REG_WRITE(offset,value)  IO_WRITE_32(OTP_MEMORY_OTP_CONTROLLER_REGISTER_MAP_BASE_ADDR+(offset), value)
#define _OTP_REG_PTR(offset)          PTR32_T(OTP_MEMORY_OTP_CONTROLLER_REGISTER_MAP_BASE_ADDR+(offset))

#define _OTP_MEMORY_READ(offset)      IO_READ_32(OTP_MEMORY_OTP_MEMORY_BLOCK_BASE_ADDR+(offset))

#define _STATUS_GET_OPMODE() (_OTP_REG_READ(OTP_CTRL_STATUS_ADDR) & OTP_CTRL_STATUS_OPMODE_MASK)

#define _STATUS_OPMODE_DISABLED            (0 << OTP_CTRL_STATUS_OPMODE_POS)
#define _STATUS_OPMODE_IDLE                (1 << OTP_CTRL_STATUS_OPMODE_POS)
#define _STATUS_OPMODE_READ_READY          (2 << OTP_CTRL_STATUS_OPMODE_POS)
#define _STATUS_OPMODE_READ_IN_PROGRESS    (3 << OTP_CTRL_STATUS_OPMODE_POS)
#define _STATUS_OPMODE_PROGRAM_IN_PROGRESS (4 << OTP_CTRL_STATUS_OPMODE_POS)
#define _STATUS_OPMODE_DEEP_STANDBY        (5 << OTP_CTRL_STATUS_OPMODE_POS)

#define _OTP_SCRAM_ITEMS (11)
#define _OTP_SCRAM_WORD_NIBBLES (8) // one u32 word holds 8 nibbles of scramble value

#define _OTP_TIMEOUT_DEFAULT 10000 // [us]

static ts_bool _otp_prog_done;

static u32 _otp_read_word(u32 addr);

static ts_bool _condition_prog_done(void)
{
    return _otp_prog_done;
}

static ts_bool _condition_is_idle(void)
{
    switch ( _STATUS_GET_OPMODE())
    {
    case _STATUS_OPMODE_IDLE:
    case _STATUS_OPMODE_READ_READY:
        return TS_TRUE;

    default:
        return TS_FALSE;
    }
}

static ts_bool _condition_is_read(void)
{
    // by first reading there is IDLE opmode
    // continuous reading cause "READ_READY" opmode

    if (( _STATUS_GET_OPMODE() == _STATUS_OPMODE_IDLE)
     || ( _STATUS_GET_OPMODE() == _STATUS_OPMODE_READ_READY))
    {
        return TS_TRUE;
    }
    return TS_FALSE;
}

void otp_init(void)
{
     _otp_prog_done = TS_FALSE;
    // NOTE: OTP use VDD - 1.2 V and VDD2 - 2.5 V which are always enabled (no need to control them)
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_OTPCLKEN_MASK);

    // OTP is by deault enabled (ENA == 1)
    if (_condition_is_idle() != TS_TRUE)
    {
        LOG_ERROR_NUM(_OTP_ERR_STATUS_MISMATCH);
        os_alarm();
    }

    // we skip OTP_CTRL_TIMING_<n>* registers and keep them in default for now, see otp_timing_init()

    // Configure timing course registers.
    _OTP_REG_WRITE(OTP_CTRL_TIMING_COURSE_ADDR, ((1 + (HW_CLOCK_MHZ / 100)) << OTP_CTRL_TIMING_COURSE_10NS_POS) | ((HW_CLOCK_MHZ) << OTP_CTRL_TIMING_COURSE_1US_POS));
    // NOTE: User should change  value of TIMING_COURSE only when STATUS[OPMODE]=DISABLED or IDLE.

    // Enable IRQ for "program done"
    _OTP_REG_WRITE(OTP_CTRL_INT_EN_ADDR, OTP_CTRL_INT_EN_PROGD_INT_EN_MASK);

    // NOTE: You need to call also otp_init_scrambling() during initialization phase 
}

void otp_timing_init(void)
{
    typedef struct {
        u32 otp_addr; // address in OTP where is stored requested value
        u32 reg_addr; // target register to write value (offset)
        u32 value;    // default value used in case no value programmed in OTP
    } otp_timing_table_t;

    const otp_timing_table_t OTP_TIMING_TABLE[] = {
        // Values according to FCP: CM_TR01_FCP_2025080800
        {LAYOUT_OTP_ADDR_TIMING+0, OTP_CTRL_TIMING_0_ADDR, 0x00000F03}, 
        {LAYOUT_OTP_ADDR_TIMING+4, OTP_CTRL_TIMING_1_ADDR, 0x0A0A0A05}, 
        {LAYOUT_OTP_ADDR_TIMING+8, OTP_CTRL_TIMING_2_ADDR, 0x00001414}, 

        {0,0,0} // end of table
    };

    int i;

    for (i = 0; ; i++)
    {
        const otp_timing_table_t *t = &OTP_TIMING_TABLE[i];
        
        if (t->otp_addr == 0)
        {
            break;
        }
        u32 value = _otp_read_word(t->otp_addr);

        if (value == OTP_EMPTY_VALUE)
        {   // no content written, use the default
            value = t->value;
        }
        _OTP_REG_WRITE(t->reg_addr, value);
    }
}

void otp_suspend(void)
{
    if (_STATUS_GET_OPMODE() != _STATUS_OPMODE_IDLE)
    {
        _OTP_REG_WRITE(OTP_CTRL_COMMAND_ADDR, OTP_CTRL_COMMAND_RRDYDIS_MASK);
        os_wait_for_critical(_condition_is_idle, _OTP_TIMEOUT_DEFAULT);
    }
    _OTP_REG_WRITE(OTP_CTRL_COMMAND_ADDR, OTP_CTRL_COMMAND_DSTDBY_MASK);
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_OTPCLKEN_MASK);
}

void otp_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_OTPCLKEN_MASK);
    _OTP_REG_WRITE(OTP_CTRL_COMMAND_ADDR, OTP_CTRL_COMMAND_WAKEUP_MASK);

}

void otp_init_scrambling(u8 *seed)
{   // <seed> is per chip fixed randomizing sequence at least 11 bytes long (number of items in SCRAM_* registers)
    u32 scram_value;
    u8 sequence[_OTP_SCRAM_ITEMS];

    OS_SANITY_NULL(seed);

    // Write OTP_CTRL_SCRAM_* registers. Sectors within a page will be re-ordered.
    //    write sequence of reordered numbers 0..11 (each number once)
    scramble_init(sequence, sizeof(sequence));
    scramble_shuffle(sequence, sizeof(sequence), seed);
    // we have prepared 11 values but we need to split them to two registers (8+3)
    scram_value = scramble_value_reversed(sequence, _OTP_SCRAM_WORD_NIBBLES);
    _OTP_REG_WRITE(OTP_CTRL_SCRAM_0_ADDR, scram_value);
     
    scram_value = scramble_value_reversed(sequence+_OTP_SCRAM_WORD_NIBBLES, _OTP_SCRAM_ITEMS-_OTP_SCRAM_WORD_NIBBLES);
    _OTP_REG_WRITE(OTP_CTRL_SCRAM_1_ADDR, scram_value);
    // NOTE: We use the "reversed" version of scramble to keep it as in ACAB where it was unintentionally reversed.
}

static u32 _otp_read_word(u32 addr)
{
    u32 read_data;

    OS_ASSERT((addr & 0x3) == 0); // only u32 word aligned address is supported
    OS_ASSERT(addr < OTP_SIZE);

    os_wait_for_critical(_condition_is_read, _OTP_TIMEOUT_DEFAULT);

    read_data = _OTP_MEMORY_READ(addr);

    return read_data;
}

u32 otp_read_word(u32 addr)
{   // harden the OTP reading, by dual reading
    u32 value1 = _otp_read_word(addr);

    // insert some random delay using fast SW pseudo RNG
    // we use busy loop because we need maximum flexibility for delay randomness 
    os_delay_cycles(prng_get_value_insecure() & 0x7F);
    // NOTE: 1ms corresponds to approx 6750 iterations in TROPIC01
    //       so mask 0x7F will add up to 19 us

    u32 value2 = _otp_read_word(addr);

    if (value1 != value2)
    {
        os_alarm();
        return 0;
    }
    return value1;
}

void otp_read_data(u8 *dest, u32 addr, size_t size)
{
    u32 w;
    size >>= 2; // bytes to words
    while (size--)
    {
        w = otp_read_word(addr);
        memcpy(dest, &w, sizeof(w));
        dest += sizeof(u32);
        addr += sizeof(u32);
    }
}


static u8 _bit_value(u8 nibble)
{   // majority of values in 4 bit nibble means the value
    int count = 0;

    while (nibble)
    {
        count += nibble & 1;
        nibble >>= 1;
    }
    // NOTE: two ones and two zeroes in nibble (count==2) is invalid value so we keep it as "0" which means more restrictive
    return ((count > 2) ? 1 : 0);
}

u8 otp_read_bit_field(u32 addr)
{   // part of OTP is "bit field" where 4 physical bits represent one real bit 
    // which give us "single bit programming" possibility (8 bits stored in one u32 word)
    // the reason for this is ECC correction which can flip one bit after multiple writes
    u32 word;
    u8 value = 0;
    u8 bit = 1;

    word = otp_read_word(addr);

    while (word)
    {
        value |= (_bit_value(word & 0xF) ? bit : 0);
        bit <<= 1;
        word >>= 4;
    }
    return value;
}

void otp_write_word(u32 addr, u32 data)
{
    OS_ASSERT((addr & 0x3) == 0); // only u32 word aligned address is supported
    OS_ASSERT(addr < OTP_SIZE);

    _otp_prog_done = TS_FALSE;
    
    _OTP_REG_WRITE(OTP_CTRL_PROG_DATA_ADDR, data);
    _OTP_REG_WRITE(OTP_CTRL_PROG_ADDR_ADDR, addr);

    _OTP_REG_WRITE(OTP_CTRL_COMMAND_ADDR, OTP_CTRL_COMMAND_PROGREQ_MASK);

    os_wait_for_critical(_condition_prog_done, _OTP_TIMEOUT_DEFAULT);
}

ts_bool otp_write_word_verify(u32 addr, u32 data)
{
    otp_write_word(addr, data);
    
    return (_otp_read_word(addr) == data) ? TS_TRUE : TS_FALSE;
}

__ISR void irq_otp_handler(void)
{
    if (_OTP_REG_READ(OTP_CTRL_STATUS_ADDR) & OTP_CTRL_STATUS_PROGD_MASK)
    {   // clear the status flag (W1C)
        _OTP_REG_WRITE(OTP_CTRL_STATUS_ADDR, OTP_CTRL_STATUS_PROGD_MASK);
        _otp_prog_done = TS_TRUE;
    }
    else
    {
        LOG_ERROR_NUM(_OTP_ERR_UNHANDLED_IRQ);
        os_alarm_isr();
    }
}

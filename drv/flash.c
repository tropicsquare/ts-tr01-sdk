/**
 * @file flash.c
 * @author Tropic Square
 * @brief Flash controller HW driver source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "bits.h"
#include "flash.h"
#include "io_ops.h"
#include "hw.h"
#include "ftc.h"
#include "scramble.h"
#include "log.h"
#include "soc_ctrl.h"
#include "fss_regs.h"
#include "tassic_defs.h"
#include "timer.h"
#include "irq_ctrl.h"
#include "util.h"

LOG_DEF("FSS");
#define _LOG_DEBUG(...) // LOG_DEBUG(__VA_ARGS__)

#define _FLASH_ERR_ECC_DED_F                1
#define _FLASH_ERR_UNHANDLED_IRQ            2
#define _FLASH_ERR_SECT_FULL                3
#define _FLASH_ERR_TRIM_FAILED              4
#define _FLASH_ERR_MAGIC                    5
#define _FLASH_ERR_ENC_SIZE                 6
#define _FLASH_ERR_STATUS                   7
#define _FLASH_COMMAND_TIMEOUT              8
#define _FLASH_ERR_ECC_SEC_F                9
#define _FLASH_ERR_ECC_SEC_R                10
#define _FLASH_ERR_ECC_DED_R                11

// FSS_FMM_BASE_ADDR,  FSS_RAM_BUF_BASE_ADDR
#define _FSS_RAM_BUF_READ(offset)           IO_READ_32(FSS_RAM_BUF_BASE_ADDR+(offset))
#define _FSS_RAM_BUF_WRITE(offset,value)    IO_WRITE_32(FSS_RAM_BUF_BASE_ADDR+(offset), value)

#define _FSS_REG_READ(offset)               IO_READ_32(FSS_REG_MAP_BASE_ADDR+(offset))
#define _FSS_REG_WRITE(offset,value)        IO_WRITE_32(FSS_REG_MAP_BASE_ADDR+(offset), value)
#define _FSS_REG_PTR(offset)                PTR32_T(FSS_REG_MAP_BASE_ADDR+(offset))

#define _CMD_MASK                           (0x3)
#define _CMD_NO_ACTION                      (2)
#define _CMD_EXEC_ACTION                    (1)
#define _CMD_INVALID_11                     (3)
#define _CMD_INVALID_00                     (0)

#define _FSS_COMMAND_NO_ACTION ( \
          (_CMD_NO_ACTION << FSS_COMMAND_PROG_ONE_POS)     | (_CMD_NO_ACTION << FSS_COMMAND_SECTOR_ERASE_POS) \
        | (_CMD_NO_ACTION << FSS_COMMAND_BLOCK_ERASE_POS)  | (_CMD_NO_ACTION << FSS_COMMAND_CHIP_ERASE_POS)   \
        | (_CMD_NO_ACTION << FSS_COMMAND_VERF_ERASE_POS)   | (_CMD_NO_ACTION << FSS_COMMAND_PROG_RAW_POS)     \
        | (_CMD_NO_ACTION << FSS_COMMAND_PROG_ENC_POS)     | (_CMD_NO_ACTION << FSS_COMMAND_READ_TO_RAM_POS)  \
        | (_CMD_NO_ACTION << FSS_COMMAND_READ_DECRYPT_POS) | (_CMD_NO_ACTION << FSS_COMMAND_FLUSH_RAM_POS)    \
        | (_CMD_NO_ACTION << FSS_COMMAND_DO_TRIM_POS)      | (_CMD_NO_ACTION << FSS_COMMAND_DO_HCK_POS)       \
        | (_CMD_NO_ACTION << FSS_COMMAND_DO_SEC_REF_POS)                                                      \
    )   // == 0x02AAAAAA


// FSS_CONFIG_PAGE defs
#define _FSS_CONFIG_PAGE_MAIN               (1 << FSS_CONFIG_PAGE_POS)
#define _FSS_CONFIG_PAGE_NVR                (2 << FSS_CONFIG_PAGE_POS)
#define _FSS_CONFIG_PAGE_REDUNDANCY         (4 << FSS_CONFIG_PAGE_POS)

#define _FSS_CONFIG_DEFAULT  ((1              << FSS_CONFIG_PAGE_POS)   | \
                              (1              << FSS_CONFIG_ECC_EN_POS) | \
                              (1              << FSS_CONFIG_VTA_EN_POS) | \
                              (_CMD_NO_ACTION << FSS_CONFIG_DEBUG_EN_POS) ) // CONFIG default value 0x00000864

#define _FSS_CONFIG_BASIC    ((1              << FSS_CONFIG_ECC_EN_POS)   | \
                              (_CMD_NO_ACTION << FSS_CONFIG_DEBUG_EN_POS) | \
                                                 FSS_CONFIG_FSS_EN_MASK)

#define _FSS_CONFIG_READ_MODE_RECALL (0x0)
#define _FSS_CONFIG_READ_MODE_NORMAL (0x3)

#define _FSS_STATUS_ERROR_ANY (FSS_STATUS_DECRYPT_ERR_MASK | \
                               FSS_STATUS_ENCRYPT_ERR_MASK | \
                               FSS_STATUS_VERF_ERR_MASK    | \
                               FSS_STATUS_ECC_DED_F_MASK   | \
                               FSS_STATUS_ECC_DED_R_MASK   | \
                               FSS_STATUS_ECC_SEC_R_MASK)

// errors which cause alarm
#define _FSS_STATUS_ERROR_CRITICAL ( \
                               FSS_STATUS_ECC_DED_F_MASK   | \
                               FSS_STATUS_ECC_DED_R_MASK   | \
                               FSS_STATUS_ECC_SEC_R_MASK)

// STATUS flags consumed by irq_flash_handler(). INT_EN mirrors the STATUS bit layout, so the
// same mask both enables the interrupts (see _flash_cfg_basic) and selects the flags the ISR
// handles - the two cannot drift apart. Enabling a contributor that is not listed here would
// make irq_flash_handler() take its unhandled branch (alarm + FIRQ1 disabled) on the first
// occurrence. That applies notably to VT_REC, which is deliberately not evaluated and which
// nothing ever clears.
#define _FSS_STATUS_IRQ_HANDLED (FSS_STATUS_OP_DONE_MASK   | \
                                 FSS_STATUS_ECC_SEC_F_MASK | \
                                 FSS_STATUS_ECC_DED_F_MASK | \
                                 FSS_STATUS_ECC_SEC_R_MASK | \
                                 FSS_STATUS_ECC_DED_R_MASK)

static_assert((FSS_STATUS_OP_DONE_MASK   == FSS_INT_EN_OP_DONE_EN_MASK)   &&
              (FSS_STATUS_ECC_SEC_F_MASK == FSS_INT_EN_ECC_SEC_F_EN_MASK) &&
              (FSS_STATUS_ECC_DED_F_MASK == FSS_INT_EN_ECC_DED_F_EN_MASK) &&
              (FSS_STATUS_ECC_SEC_R_MASK == FSS_INT_EN_ECC_SEC_R_EN_MASK) &&
              (FSS_STATUS_ECC_DED_R_MASK == FSS_INT_EN_ECC_DED_R_EN_MASK),
              "FSS INT_EN does not mirror the STATUS bit layout");

#define _FSS_STATUS_TIMEOUT_MAX 10000 // [us]
#define _FSS_STATUS_TIMEOUT_DEF  1000 // [us]

#define _FLASH_ISR_TIMEOUT (115000) // number of iterations to get approx 10ms

#define _FSS_SECTOR_SCRAM_ITEMS    6
#define _FSS_PAGE_SCRAM_ITEMS      7
// NOTE: we have physically 2^10 pages (sectors), but MSB 3 bits are fixed (without scrambling)
//       The reason is to avoid remap i.e. MACANDD to CPU accessible space.
#define _FSS_WORD_SCRAM_SIZE       8 // number of items which fit into one configuration word

static const u32 FL_MAGIC_WORD = 0xF0A55A0F;
#define _TRIM_DEFAULT_VALUE (0x0000FFFF)

// Structure of encrypted sector defined by HW
typedef struct {
    u8      data[FLASH_MAX_ENCRYPTED_SIZE];
    u8      nonce[FLASH_NONCE_SIZE];
    u8      atag[FLASH_ATAG_SIZE];
    u16     size;
    u8      reserved;
    u8      status;
} __PACK __ALIGN_U32 flash_enc_sector_t; // sizeof(flash_enc_sector_t) = FLASH_SECTOR_SIZE

static_assert(sizeof(flash_enc_sector_t) == FLASH_SECTOR_SIZE, "sizeof(flash_enc_sector_t) != FLASH_SECTOR_SIZE");
static_assert(FLASH_MAX_ENCRYPTED_PAYLOAD < FLASH_MAX_ENCRYPTED_SIZE,
              "encrypted payload limit must fit inside SECT_CTEXT");

typedef struct {
    union {
        u32                 data32[FLASH_SECTOR_SIZE/sizeof(u32)];
        u8                  data8[FLASH_SECTOR_SIZE];
        flash_enc_sector_t  enc;
    };
} flash_sector_t;

static_assert(sizeof(flash_sector_t) == FLASH_SECTOR_SIZE, "sizeof(flash_sector_t) != FLASH_SECTOR_SIZE");

static flash_sector_t _sector_cache;

static u32 _fss_config_reg_cache = 0; // FSS_CONFIG_ADDR register cache to reduce read/modify/write
static volatile ts_bool _op_done; // ISR controlled
static volatile ts_bool _op_error; // ISR controlled
static ts_bool _flash_init_done;

static TS_CHECK_RETVAL ts_bool _condition_op_done(void)
{
    return _op_done;
}

static TS_CHECK_RETVAL ts_bool _condition_status_active(void)
{
    return FIELD_GET(FSS_STATUS_ACTIVE_MASK, _FSS_REG_READ(FSS_STATUS_ADDR)) ? TS_TRUE : TS_FALSE;
}

static TS_CHECK_RETVAL ts_bool _condition_idle(void)
{
    return FIELD_GET(FSS_STATUS_IDLE_MASK, _FSS_REG_READ(FSS_STATUS_ADDR)) ? TS_TRUE : TS_FALSE;
}

static void _fss_wait_for_idle(void)
{
    os_wait_for_critical(_condition_idle, _FSS_STATUS_TIMEOUT_MAX); 
}

/**
 * @brief Wait for idle using busy loop.
 *
 * Usable when os_wait_for* cant work. I.e called from ISR.
 */
static TS_CHECK_RETVAL ts_bool _fss_wait_for_idle_isr(void)
{
    u32 retries = _FLASH_ISR_TIMEOUT;

    while (_condition_idle() != TS_TRUE)
    {
        if (--retries == 0)
        {
            return TS_FALSE;
        }
    }
    return TS_TRUE;
}

/**
 * @brief Execute Command on Flash Subsystem
 * @param[in] cmd_pos bit position in COMMAND register (each command has its own position)
 */
static void _fss_command_exec(u8 cmd_pos)
{
    _fss_wait_for_idle();

    _op_done = TS_FALSE;

    // _CMD_EXEC_ACTION == _CMD_NO_ACTION ^ _CMD_MASK
    _FSS_REG_WRITE(FSS_COMMAND_ADDR, _FSS_COMMAND_NO_ACTION ^ (_CMD_MASK << cmd_pos));
}

static TS_CHECK_RETVAL ts_bool _fss_errors_handle(void)
{
    u32 status = _FSS_REG_READ(FSS_STATUS_ADDR);

    // NOTE: we handle ECC_SEC_R as error, because RAM buffer should work without ECC issues.
    if (status & _FSS_STATUS_ERROR_ANY)
    {
        _LOG_DEBUG("ST: %x", status);
        // Clear all errors
        _FSS_REG_WRITE(FSS_STATUS_ADDR, status & _FSS_STATUS_ERROR_ANY);
        if (status & _FSS_STATUS_ERROR_CRITICAL)
        {
            LOG_ERROR_NUM(_FLASH_ERR_STATUS);
            os_alarm();
        }
        // continue in case non fatal error i.e. DECRYPT_ERR
        return TS_FALSE;
    }

    // Check IRQ handled error
    if (_op_error == TS_TRUE)
    {
        _op_error = TS_FALSE;
        os_alarm();
    }
    return TS_TRUE;
}

static TS_CHECK_RETVAL ts_bool _fss_command(u8 cmd_pos)
{
    _fss_command_exec(cmd_pos);

    // Wait for it is done
    os_wait_for_critical(_condition_op_done, _FSS_STATUS_TIMEOUT_MAX);

    return _fss_errors_handle();
}

inline static void _flash_cfg_basic(void)
{
    // Configure prescalers
    // NOTE: we keep TIMING_* registers in default
    u32 course = FIELD_PREP(FSS_TIMING_COURSE_10NS_MASK, (1 + (HW_CLOCK_MHZ / 100))) |
                 FIELD_PREP(FSS_TIMING_COURSE_1US_MASK, HW_CLOCK_MHZ);
    _FSS_REG_WRITE(FSS_TIMING_COURSE_ADDR, course);

    // Set CONFIG[FSS_EN] = 1
    _FSS_REG_PTR(FSS_CONFIG_ADDR) |= FSS_CONFIG_FSS_EN_MASK;

    // Wait until STATUS[IDLE] = 1.
    _fss_wait_for_idle();

    // Write the rest of the CONFIG register
    // (Needed as CONFIG[PAGE] is read-only when STATUS[IDLE]=0).
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _FSS_CONFIG_BASIC     |
                                    _FSS_CONFIG_PAGE_MAIN |
                                    FSS_CONFIG_VTA_EN_MASK);

    // Wait until STATUS[ACTIVE] = 1.
    os_wait_for_critical(_condition_status_active, _FSS_STATUS_TIMEOUT_MAX);

    // Enable interrupts (INT_EN mirrors the STATUS bit layout, see _FSS_STATUS_IRQ_HANDLED)
    _FSS_REG_WRITE(FSS_INT_EN_ADDR, _FSS_STATUS_IRQ_HANDLED);

    // Load register cache (mirrors real CONFIG; READ_MODE stays Recall until
    // trim is done - see _flash_cfg_trim).
    _fss_config_reg_cache = _FSS_REG_READ(FSS_CONFIG_ADDR);
}

inline static void _flash_cfg_trim(void)
{
    // Read NVR sector to RAM buffer
    // Ok to ignore retval for now, as the function can only fail
    // on bad arguments. For now, FLASH_NVR0_ADDR passes the check.
    TS_IGNORE_RESULT(flash_read_nvr_to_buf(FLASH_NVR0_ADDR));

    // Check if NVR0.FL_MAGIC_WORD == 0xF0A55A0F, (see section Flash Memory NVR page layout in [4]).
    //     If NVR0.FL_MAGIC_WORD != 0xF0A55A0F, then EAHBM shall write all addresses
    //     in RAM Buffer that correspond to NVR0.FL_TRIM_DATA* with 0x0000FFFF. This
    //     will apply default trim values in case where NVR0 Sector does not contain valid
    //     FMM trim values. For details see section Flash Memory NVR page layout in [4].
    if (_FSS_RAM_BUF_READ(FLASH_NVR0_OFFSET_MAGIC_WORD) != FL_MAGIC_WORD)
    {
        LOG_ERROR_NUM(_FLASH_ERR_MAGIC);
        _LOG_DEBUG("MAGIC: %x", _FSS_RAM_BUF_READ(FLASH_NVR0_OFFSET_MAGIC_WORD));

        for (u32 addr = FLASH_NVR0_OFFSET_TRIM_DATA0; addr <= FLASH_NVR0_OFFSET_TRIM_DATA7; addr += sizeof(u32))
        {
            _FSS_RAM_BUF_WRITE(addr, _TRIM_DEFAULT_VALUE);
        }
    }

    // Write COMMAND[DO_TRIM]=0x1 and wait until STATUS[OP_DONE]=1.
    // Ok to ignore retval; TRIM_STS handling is not covered _fss_command.
    // We handle TRIM_STS below.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_DO_TRIM_POS));

    // EAHBM checks STATUS[TRIM_STS]=1. If not, maybe vdd_* not enabled.
    if ((_FSS_REG_READ(FSS_STATUS_ADDR) & FSS_STATUS_TRIM_STS_MASK) == 0)
    {
        LOG_ERROR_NUM(_FLASH_ERR_TRIM_FAILED);
        os_alarm();
    }

    // EAHBM clears STATUS[OP_DONE].
    _FSS_REG_WRITE(FSS_STATUS_ADDR, FSS_STATUS_OP_DONE_MASK);

    // Trim done -> Normal read mode is now safe.
    FIELD_SET(_fss_config_reg_cache, FSS_CONFIG_READ_MODE_MASK, _FSS_CONFIG_READ_MODE_NORMAL);

    // Restore config register (now with Normal read mode).
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);
}

static void _copy_sector_to_ram_buf(u32 *data)
{
    for (int i=0; i<FLASH_SECTOR_SIZE; i+=sizeof(u32), data++)
    {
        _FSS_RAM_BUF_WRITE(i, *data);
    }
}

static void _copy_sector_from_ram_buf(u32 *dest)
{
    for (int i = 0; i < FLASH_SECTOR_SIZE; i += sizeof(u32), dest++)
    {
        *dest = _FSS_RAM_BUF_READ(i);
    }

    // Ok to ignore retval; only ECC_SEC_R or ECC_DED_R can occur and we handle those with alarm.
    TS_IGNORE_RESULT(_fss_errors_handle());
}

static void _flush_ram_buf(void)
{
    for (int i = 0; i < FLASH_SECTOR_SIZE; i += sizeof(u32))
    {
        _FSS_RAM_BUF_WRITE(i, 0);
    }
}

void flash_init(void)
{
    // Set to default according to HW
    _fss_config_reg_cache = _FSS_CONFIG_DEFAULT;
    _op_done = TS_FALSE; 
    _op_error = TS_FALSE;
    
    flash_wakeup();
    _flash_init_done = TS_TRUE; // NOTE: before init we have here 0 (not TS_FALSE)
}

ts_bool flash_init_done(void)
{
    return _flash_init_done;
}

void flash_suspend(void)
{
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache & ~FSS_CONFIG_FSS_EN_MASK);

    // Disable clock to FLASH controller
    soc_ctrl_clk_dis(SOC_CTRL_CLK_FSS);

    // Disable the power to FMM
    // NOTE: there must be switched off FSS_CONFIG_FSS_EN before SOC_CTRL_PWR_ENA_* manipulation
    soc_ctrl_pwr_off(SOC_CTRL_PWR_ENA_VDD_1V7_0_ENA_MASK |
                     SOC_CTRL_PWR_ENA_VDD_1V7_1_ENA_MASK |
                     SOC_CTRL_PWR_ENA_VREF_1V0_ENA_MASK);

}

void flash_wakeup(void)
{
    // Enable clock to FLASH controller
    soc_ctrl_clk_en(SOC_CTRL_CLK_FLASH_SUBSET);

    // Enable the power to FMM on a system level
    // (vdd_1v7_0, vdd_1v7_1 and vdd_1v0 must be enabled, therefore all bits of power_state must be 1).
    soc_ctrl_pwr_on(SOC_CTRL_PWR_ENA_VDD_1V7_0_ENA_MASK |
                    SOC_CTRL_PWR_ENA_VDD_1V7_1_ENA_MASK |
                    SOC_CTRL_PWR_ENA_VREF_1V0_ENA_MASK);

    // According to vdd_1v7_1 LDO documentation, after vdd_1v7_0_ena rises,
    // it takes up to 120 us to finish its power-up!
    os_delay_us(120);

    // Restore config register (enabled again if already set in cache)
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);

    // we need to do all the power-on stuff including trimming, because VDD power was off
    _flash_cfg_basic();
    _flash_cfg_trim();
}

void flash_ecc_disable(void)
{
    _fss_config_reg_cache &= ~FSS_CONFIG_ECC_EN_MASK;
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);
}

void flash_ecc_enable(void)
{
    _fss_config_reg_cache |= FSS_CONFIG_ECC_EN_MASK;
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);
}


void flash_init_scrambling(u8 *seed)
{
    // <seed> is (per chip fixed) randomizing sequence at least
    // _FSS_SECTOR_SCRAM_ITEMS + _FSS_PAGE_SCRAM_ITEMS bytes long
    u8 sequence[_FSS_WORD_SCRAM_SIZE] = {0};

    // Write FSS_SECTOR_SCRAM register. Words within a sector will be re-ordered.
    //    write sequence of reordered numbers 0..5 (each number once)
    // NOTE: here exist 7-th fixed item 0x6 which is RO (implementation limit)
    scramble_init(sequence, _FSS_SECTOR_SCRAM_ITEMS);
    scramble_shuffle(sequence, _FSS_SECTOR_SCRAM_ITEMS, seed);
    _FSS_REG_WRITE(FSS_SECTOR_SCRAM_ADDR, scramble_value(sequence, _FSS_SECTOR_SCRAM_ITEMS));

    // Write FSS_PAGE_SCRAM_* registers. Sectors within a page will be re-ordered.
    //    write sequence of reordered numbers 0..6 (each number once)
    scramble_init(sequence, _FSS_WORD_SCRAM_SIZE); // init for whole word (not only _FSS_PAGE_SCRAM_ITEMS), to keep ACAB compatibility
    scramble_shuffle(sequence, _FSS_PAGE_SCRAM_ITEMS, seed + _FSS_SECTOR_SCRAM_ITEMS);
    _FSS_REG_WRITE(FSS_PAGE_SCRAM_0_ADDR, scramble_value(sequence, _FSS_WORD_SCRAM_SIZE));
    // The FSS_PAGE_SCRAM_1 and PAGE_SCRAM_0[ADDR7] contains RO constant values

    //  NOTE: If CPU tried to access NVR page or Redundancy page, the FSS automatically
    //        over-rides the scrambling and uses unscrambled addresses regardless of
    //        registers configuration (SECTOR_SCRAM and PAGE_SCRAM*).

    // erase the scrambling from RAM after its written to regs
    memerase_safe(sequence, sizeof(sequence));
}

// WARNING: The function returns 0x0 as data if flash_read_data() fails.
// All code paths that exist at this commit are verified to not be affected by this warning.
u32 flash_read_word(u32 address)
{
    u32 word = 0;
    
    if (flash_read_data(&word, address, sizeof(u32)) == TS_TRUE)
    {
        return (word);
    }

    return (0);
}

ts_bool flash_read_data(u32 *dest, u32 address, size_t size)
{
    if (((address + size) >= FLASH_SIZE) || (address & 0x3) || (size & 0x3))
    {
        return TS_FALSE; // Out of bounds or non-aligned size
    }

    _fss_wait_for_idle();

    // Switch ON "direct read access mode"
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache | FSS_CONFIG_DRR_EN_MASK);

    while (size>3)
    {
        // Read single word (unencrypted) from FMM via AHB
        // The content is accessible from FSS_FMM_BASE_ADDR
        *dest = IO_READ_32(FSS_FMM_BASE_ADDR+address);
        // NOTE: The AHB transfer that reads from the FMM will be stalled for significant amount of
        //       time (100-200ns). Therefore, if EAHBM needs perform multiple read operations over
        //       one sector, it is recommended to perform read to ram of given sector first.
    
        dest++;
        size -= sizeof(u32);
        address += sizeof(u32);
    }

    // Switch OFF "direct read access mode"
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);
    return TS_TRUE;
}

void flash_write_word(u32 address, u32 data)
{
    if ((address >= FLASH_SIZE) || (address & 0x3))
    {
        return; // out of bounds
    }
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    _FSS_REG_WRITE(FSS_PROG_DATA_ADDR, data);
    // Ok to ignore retval; only OP_AUTH_ERR can occur, but we don't handle it.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_PROG_ONE_POS));
}

// WARNING: If `data` is 0x0 and flash_read_word() fails, the function returns TS_TRUE.
// All code paths that exist at this commit are verified to not be affected by this warning.
ts_bool flash_write_word_verify(u32 address, u32 data)
{
    flash_write_word(address, data);

    return (flash_read_word(address) == data) ? TS_TRUE : TS_FALSE;
}

void flash_write_word_isr(u32 address, u32 data)
{
    if ((address >= FLASH_SIZE) || (address & 0x3))
    {
        return; // out of bounds
    }
    
    if (_fss_wait_for_idle_isr() != TS_TRUE)
    {
        return;
    }

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    _FSS_REG_WRITE(FSS_PROG_DATA_ADDR, data);
    
    if (_fss_wait_for_idle_isr() != TS_TRUE)
    {
        return;
    }
    _FSS_REG_WRITE(FSS_COMMAND_ADDR, _FSS_COMMAND_NO_ACTION ^ (_CMD_MASK << FSS_COMMAND_PROG_ONE_POS));
}

// WARNING: If `data` is 0x0 and flash_read_word() fails, the function returns TS_TRUE.
// All code paths that exist at this commit are verified to not be affected by this warning.
ts_bool flash_safe_write_word(u32 address, u32 data)
{
    u32 content = flash_read_word(address);

    if (content == data)
    {
        return (TS_TRUE); // the same value already written, its OK
    }
    if (content != FLASH_EMPTY_VALUE)
    {
        return (TS_FALSE); // cant write, no free space
    }
    if (data == FLASH_EMPTY_VALUE)
    {
        return (TS_TRUE); // no need to write, it would write ECC only
    }
    return (flash_write_word_verify(address, data));
}

ts_bool flash_read_sector(u32 *dest, u32 address)
{
    if ((address >= FLASH_SIZE) || (address & FLASH_SECTOR_MASK))
    {
        return TS_FALSE; // out of bounds
    }
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);

    // Ok to ignore retval; only these can occur:
    // 1. ECC_SEC_F -> ok, is corrected.
    // 2. ECC_DED_F -> we go to alarm.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_READ_TO_RAM_POS));

    _copy_sector_from_ram_buf(dest);
    return TS_TRUE;
}

void flash_clear_sector_cache(void)
{
    memerase_safe(&_sector_cache, sizeof(_sector_cache));
}

size_t flash_read_sector_enc(u8 *dest, size_t dest_size, u32 address)
{
    flash_enc_sector_t *sector = &_sector_cache.enc;

    if ((address >= FLASH_SIZE) || (address & FLASH_SECTOR_MASK))
    {
        return 0; // out of bounds
    }
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    if (_fss_command(FSS_COMMAND_READ_DECRYPT_POS) != TS_TRUE)
    {
        return (0);
    }
    _copy_sector_from_ram_buf((u32 *)sector);
    _flush_ram_buf();

    if (sector->size > FLASH_MAX_ENCRYPTED_PAYLOAD)
    {
        memerase_safe(sector, sizeof(*sector)); // clear plaintext data
        LOG_ERROR_NUM(_FLASH_ERR_ENC_SIZE);
        _LOG_DEBUG("size mismatch %d", sector->size);
        return 0;
    }

    size_t copy_cnt = (sector->size > dest_size) ? dest_size : sector->size;
    memcpy(dest, sector->data, copy_cnt);
    memerase_safe(sector, sizeof(*sector)); // clear plaintext data
    return copy_cnt;
}

// WARNING: if the cases when this function can fail change,
// take it into account in _flash_cfg_trim(), where the retval is ignored.
ts_bool flash_read_nvr_to_buf(u32 address)
{
    if ((address >= FLASH_SIZE) || (address & FLASH_SECTOR_MASK))
    {
        return TS_FALSE; // out of bounds
    }
    _fss_wait_for_idle();

    // Set CONFIG[READ_MODE] to Recall read and CONFIG[PAGE] to NVR page.
    // NOTE: CONFIG_VTA_EN must be switched OFF
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _FSS_CONFIG_BASIC |
                                    FIELD_PREP(FSS_CONFIG_READ_MODE_MASK, _FSS_CONFIG_READ_MODE_RECALL) |
                                    _FSS_CONFIG_PAGE_NVR);

    // Set ADDRESS
    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);

    // Read data from the FMM to the RAM buffer
    // Ok to ignore retval; only these can occur:
    // 1. ECC_SEC_F -> ok, is corrected.
    // 2. ECC_DED_F -> we go to alarm.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_READ_TO_RAM_POS));

    // Restore config register
    _FSS_REG_WRITE(FSS_CONFIG_ADDR, _fss_config_reg_cache);
    return TS_TRUE;
}

ts_bool flash_read_nvr(u32 *dest, u32 address)
{
    if (flash_read_nvr_to_buf(address) != TS_TRUE)
    {
        return TS_FALSE;
    }
    _copy_sector_from_ram_buf(dest);
    return TS_TRUE;
}

ts_bool flash_write_sector(u32 address, u32 *data)
{
    if ((address >= FLASH_SIZE) || (address & FLASH_SECTOR_MASK))
    {
        return TS_FALSE; // out of bounds
    }
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);

    // Copy data to RAM buffer
    _copy_sector_to_ram_buf(data);

    // Set bitmask which words to program (all of them)
    _FSS_REG_WRITE(FSS_PROG_MASK_31_0_ADDR, UINT32_MAX);
    _FSS_REG_WRITE(FSS_PROG_MASK_63_32_ADDR, UINT32_MAX);
    _FSS_REG_WRITE(FSS_PROG_MASK_95_64_ADDR, UINT32_MAX);
    _FSS_REG_WRITE(FSS_PROG_MASK_127_96_ADDR, UINT32_MAX);

    // Ok to ignore retval; when FSS copies data from RAM buffer to Flash,
    // only ECC from RAM buffer can fail and we handle it with alarm.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_PROG_RAW_POS));
    return TS_TRUE;
}

void flash_set_nonce(u8 nonce[FLASH_NONCE_SIZE])
{
    sys_copy_mem_to_regs(FSS_REG_MAP_BASE_ADDR + FSS_ISAP_NONCE_0_ADDR, nonce, FLASH_NONCE_SIZE);
}

ts_bool flash_write_sector_enc(u32 address, void *data, size_t size, u8 nonce[FLASH_NONCE_SIZE])
{
    flash_enc_sector_t *sector = &_sector_cache.enc;

    if ((address >= FLASH_SIZE)        ||
        (address & FLASH_SECTOR_MASK) ||
        (size  > FLASH_MAX_ENCRYPTED_PAYLOAD))
    {
        return TS_FALSE; // out of bounds
    }

    _fss_wait_for_idle();

    memset(sector, 0, sizeof(*sector));
    memcpy(sector->data, data, size);
    flash_set_nonce(nonce);
    sector->size = (u16)size;

    _copy_sector_to_ram_buf((u32 *)sector);
    // clear buffer after use, may be sensitive data in RAM
    memerase_safe(sector, sizeof(*sector)); // clear plaintext data
    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);

    if (_fss_command(FSS_COMMAND_PROG_ENC_POS) != TS_TRUE)
    {
        _flush_ram_buf();          // clear plaintext on failure
        LOG_ERROR_NUM(_FLASH_ERR_SECT_FULL);
        return TS_FALSE;
    }

    _flush_ram_buf();          // clear plaintext after command is done
    return TS_TRUE;
}

ts_bool flash_verify_erased(u32 address)
{
    if ((address >= FLASH_SIZE) || (address & FLASH_SECTOR_MASK))
    {
        return TS_FALSE;
    }
    _fss_wait_for_idle();

    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    return (_fss_command(FSS_COMMAND_VERF_ERASE_POS));
}

void flash_erase_sector(u32 address)
{
    _fss_wait_for_idle();
    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    // Ok to ignore retval; erase can't fail.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_SECTOR_ERASE_POS));
}

void flash_erase_block(u32 address)
{
    _fss_wait_for_idle();
    _FSS_REG_WRITE(FSS_ADDRESS_ADDR, address);
    // Ok to ignore retval; erase can't fail.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_BLOCK_ERASE_POS));
}

void flash_erase_chip(void)
{
    // Ok to ignore retval; erase can't fail.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_CHIP_ERASE_POS));
}

void flash_flush_rambuf(void)
{
    // Ok to ignore retval; RAM flush can't fail.
    TS_IGNORE_RESULT(_fss_command(FSS_COMMAND_FLUSH_RAM_POS));
}

/**
 * @brief Flash Subsystem interrupt handler.
 *
 * int_fss is a level signal (the OR of STATUS & INT_EN), so every enabled flag is serviced
 * even if it is raised while this handler runs - the handler is simply re-entered. All flags
 * present in one status snapshot are handled in a single pass, fatal ones first.
 */
__ISR void irq_flash_handler(void)
{
    const u32 fss_status_reg = _FSS_REG_READ(FSS_STATUS_ADDR);
    const u32 handled_irqs   = fss_status_reg & _FSS_STATUS_IRQ_HANDLED;
    ts_bool   alarm          = TS_FALSE;

    // Other interrupt causes unhandled -> Disable and go to alarm
    if (handled_irqs == 0)
    {
        LOG_ERROR_NUM(_FLASH_ERR_UNHANDLED_IRQ);
        cpu_disable_interrupt(CSR_MIE_FIRQ1E);
        os_alarm_isr();
        return;
    }

    // Both ECC_SEC_R and ECC_DED_R:
    // - should not occur under standard operation, only during attack,
    // - can also be raised during flash operations executed by other HW blocks.
    if (handled_irqs & FSS_STATUS_ECC_DED_R_MASK)
    {
        LOG_ERROR_NUM(_FLASH_ERR_ECC_DED_R);
        alarm = TS_TRUE;
    }
    if (handled_irqs & FSS_STATUS_ECC_SEC_R_MASK)
    {
        LOG_ERROR_NUM(_FLASH_ERR_ECC_SEC_R);
        alarm = TS_TRUE;
    }

    // Both ECC_SEC_F and ECC_DED_F can also be raised during flash operations
    // executed by other HW blocks.
    // ECC_DED_F is not recoverable (double error detection) -> go to alarm.
    if (handled_irqs & FSS_STATUS_ECC_DED_F_MASK)
    {
        LOG_ERROR_NUM(_FLASH_ERR_ECC_DED_F);
        alarm = TS_TRUE;
    }
    // ECC_SEC_F is recoverable (single error correction) -> only log.
    if (handled_irqs & FSS_STATUS_ECC_SEC_F_MASK)
    {
        LOG_ERROR_NUM(_FLASH_ERR_ECC_SEC_F);
    }
    // Operation done detected.
    if (handled_irqs & FSS_STATUS_OP_DONE_MASK)
    {
        _op_done = TS_TRUE;
    }

    if (alarm != TS_FALSE)
    {
        _op_error = TS_TRUE;
        os_alarm_isr();
    }

    // Clear exactly the flags observed above.
    _FSS_REG_WRITE(FSS_STATUS_ADDR, handled_irqs);
}


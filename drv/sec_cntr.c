/**
 * @file sec_cntr.c
 * @author Tropic Square
 * @brief Security Center
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <string.h>

#include "io_ops.h"
#include "os.h"
#include "log.h"
#include "secclk.h"


#include "tassic_defs.h"
#include "sec_cntr_regs.h"
#include "soc_ctrl_regs.h"
#include "shield_regs.h"

#include "sec_cntr.h"

LOG_DEF("SCNTR");

#define _SHLD1 BDRK_SHLD_BASE_ADDR
#define _SHLD2 BDRK_SHLD2_BASE_ADDR

#define _SEC_CNTR_REG_READ(offset)          IO_READ_32(TROPIC01_MEMORY_MAP_SCNTR_BASE_ADDR+(offset))
#define _SEC_CNTR_REG_WRITE(offset,value)   IO_WRITE_32(TROPIC01_MEMORY_MAP_SCNTR_BASE_ADDR+(offset), value)

#define _SOCCTRL_REG_READ(offset)           IO_READ_32(TROPIC01_MEMORY_MAP_SOCCTRL_BASE_ADDR+(offset))
#define _SOCCTRL_REG_WRITE(offset,value)    IO_WRITE_32(TROPIC01_MEMORY_MAP_SOCCTRL_BASE_ADDR+(offset), value)

#define _SHIELD_REG_READ(ba,offset)            IO_READ_32((ba)+(offset))
#define _SHIELD_REG_WRITE(ba,offset,value)     IO_WRITE_32((ba)+(offset), value)

#define _TS_ACTIVATION_TIME_US (50)

#define _PROVISION_CTRL_SEQ1  (0xDEADBEEF)
#define _PROVISION_CTRL_SEQ2  (0xCAFE4FEE)
#define _PROVISION_CTRL_SEQ3  (0x00BA0BAB)

/** @brief Mask of already enabled sensors */
static u64 _sensors_enabled;

/** @brief Mask of sensors caused memory */
static volatile u64 _alarm_memory;

/**
 * @brief Configure shield IP as in Shield specification.
 *
 * @note We don't do a separate driver for Shield since it is only processed by
 *       Security Center Alarm logic.
 */
static void _shield_init(void)
{
    // Enable the IPs
    u32 ctrl = FIELD_PREP(SHIELD_CTRL_SCLK_ENA_MASK, 0x2);
    _SHIELD_REG_WRITE(_SHLD1, SHIELD_CTRL_ADDR, ctrl);
    _SHIELD_REG_WRITE(_SHLD2, SHIELD_CTRL_ADDR, ctrl);

    // Enable all shield alarms
    u32 tmp = FIELD_PREP(SHIELD_AL_ENA_ALARM_ENA0_MASK, 1) |
              FIELD_PREP(SHIELD_AL_ENA_ALARM_ENA1_MASK, 1) |
              FIELD_PREP(SHIELD_AL_ENA_ALARM_ENA_INPUT_MASK, 1);
    _SHIELD_REG_WRITE(_SHLD1, SHIELD_AL_ENA_ADDR, tmp);
    _SHIELD_REG_WRITE(_SHLD2, SHIELD_AL_ENA_ADDR, tmp);
}

static void sec_cntr_configure_unmask(u64 unmask)
{
    u32 low = (u32)unmask;
    u32 high = unmask >> 32;

    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_UNMASK_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_UNMASK_2_ADDR, high);
}

static void _sec_cntr_assert_configured(void)
{   // Do some basic check that sensors are enabled and configuration match expectation.
    // We expect all sensors enabled since sec_cntr_init()
    u32 tmp =
        FIELD_PREP(SEC_CNTR_CONFIG_MON_VCC_ENA_MASK,            0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_TS_ENA_MASK,                 0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_LD_RESET_N_MASK,             0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_LD_SET_N_MASK,               0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_EMPD_SET_N_MASK,             0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_EMPD_BAD_INIT_DATA_MASK,     0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_PTRNG0_RED_EN_MASK,          0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_PTRNG1_RED_EN_MASK,          0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_SHIELD_RED_EN_MASK,          0x1);

  #define CONFIG_CHECK_MASK ~(SEC_CNTR_CONFIG_GPEN_MASK | SEC_CNTR_CONFIG_PES_MASK)
    OS_ASSERT((_SEC_CNTR_REG_READ(SEC_CNTR_CONFIG_ADDR) & CONFIG_CHECK_MASK) == tmp);

    u64 channels = _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_UNMASK_2_ADDR);
    channels <<= 32;
    channels += _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_UNMASK_1_ADDR);

    OS_ASSERT(channels == _sensors_enabled);

    channels = _SEC_CNTR_REG_READ(SEC_CNTR_INT_ENA_A_2_ADDR);
    channels <<= 32;
    channels += _SEC_CNTR_REG_READ(SEC_CNTR_INT_ENA_A_1_ADDR);

    OS_ASSERT(channels == _sensors_enabled);
}

void sec_cntr_configure_interrupts(u64 channels)
{
    u32 low = (u32)channels;
    u32 high = channels >> 32;

    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_A_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_A_2_ADDR, high);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_B_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_B_2_ADDR, high);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_C_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_INT_ENA_C_2_ADDR, high);
}

void sec_cntr_init(const sec_cntr_config_t *config)
{
    OS_SANITY_NULL(config);

    sec_cntr_wakeup();

    _shield_init();

    // Configure Sensors, Step 1
    //      - Enable voltage monitor
    //      - Enable Temperature Sensor
    //      - Reset Laser detector
    //      - Reset EM Pulse detector
    //      - Enable redundancy on TRNG0,1 and shield
    //
    // NOTE: Secure clock frequency monitor is enabled in secclk_trim() function

    u32 tmp =
        FIELD_PREP(SEC_CNTR_CONFIG_MON_VCC_ENA_MASK,            0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_TS_ENA_MASK,                 0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_LD_RESET_N_MASK,             0x0)    |
        FIELD_PREP(SEC_CNTR_CONFIG_LD_SET_N_MASK,               0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_EMPD_SET_N_MASK,             0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_EMPD_INIT_ENA_MASK,          0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_EMPD_BAD_INIT_DATA_MASK,     0x0)    |
        FIELD_PREP(SEC_CNTR_CONFIG_PTRNG0_RED_EN_MASK,          0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_PTRNG1_RED_EN_MASK,          0x1)    |
        FIELD_PREP(SEC_CNTR_CONFIG_SHIELD_RED_EN_MASK,          0x1);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    // Configure Sensors Step 2:
    //      - Unreset Laser Detector
    //      - Unreset EM Pulse detector
    FIELD_SET(tmp, SEC_CNTR_CONFIG_LD_RESET_N_MASK,             0x1);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_INIT_ENA_MASK,          0x0);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    // Configure Sensors Step 3:
    //      - Finish EMPD init by setting bad_init_data = 1
    FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_BAD_INIT_DATA_MASK,     0x1);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    // Wait after configuring the sensors. Temperature sensor needs 50 us of wait
    // time before it gets activated! We add some reserve.
    os_delay_us(_TS_ACTIVATION_TIME_US * 2);

    // Clear all active alarms in Alarm Channels apart from Life-cycle controller.
    // This is to handle spuriously set alarms due to non-initialized IPs.
    // Life-cycle controller alarm is set by HW though before FW is launched and
    // someone would tamper eFuse and OTP outputs.
    u64 alm_clr = UINT64_MAX;
    alm_clr &= ~(SEC_CNTR_ALM_CHNL_LIFE_CYCLE);
    sec_cntr_clr_alarms(alm_clr);

    // Configure resets
    u32 low = (u32)config->rst_en;
    u32 high = config->rst_en >> 32;
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_RST_EN_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_RST_EN_2_ADDR, high);

    low = (u32)config->rst_mask;
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_RST_MASK_ADDR, low);

    // Configure MBIST
    low = (u32)config->mbist_en;
    high = config->mbist_en >> 32;
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_MBIST_EN_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_MBIST_EN_2_ADDR, high);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_MBIST_CHANNELS_ADDR, config->mbist_channels);

    // Configure refresh wait time
    _SEC_CNTR_REG_WRITE(SEC_CNTR_REFRESH_WAIT_TIME_ADDR, config->refresh_wait_time);

    // Configure precharge
    sec_cntr_precharge_en(config->precharge_en, config->precharge_prescaler ,SEC_CNTR_PRECHARGE_SRC_PTRNG0_ARB);

    // Enable selected sensors
    sec_cntr_configure_interrupts(config->int_en);
    sec_cntr_configure_unmask(config->int_en);

    _sensors_enabled = config->int_en;

    // re-check config written
    _sec_cntr_assert_configured();
}

void sec_cntr_init_app(void)
{
    u64 channels = _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_UNMASK_2_ADDR);
    channels <<= 32;
    channels += _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_UNMASK_1_ADDR);

    _sensors_enabled = channels;

    _sec_cntr_assert_configured();
}

void sec_cntr_set_active_sensors(u64 channels)
{
    _sec_cntr_assert_configured();

    // clear alarm of newly enabled sensors if present
    sec_cntr_clr_alarms(channels & ~_sensors_enabled);
    _sensors_enabled = channels;

    // Enable alarm interrupts and allow sensors to fire alarm channel
    sec_cntr_configure_interrupts(channels);
    sec_cntr_configure_unmask(channels);
}

u64 sec_cntr_get_active_sensors(void)
{
    return _sensors_enabled;
}


void sec_cntr_wakeup(void)
{
    // Enable clock for:
    //  - Security Center
    //  - EM Pulse detector - No dedicated driver for this, rest of the EMPD
    //    control is done by Security Center, so keep it here!
    //  - Shield and Shield 2 - No dedicated driver, controlled only through this driver
    u32 tmp = _SOCCTRL_REG_READ(SOC_CTRL_CLK_EN_ADDR);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_SECCNTRCLKEN_MASK, 1);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_EMCLKEN_MASK, 1);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_SHLDCLKEN_MASK, 1);
    _SOCCTRL_REG_WRITE(SOC_CTRL_CLK_EN_ADDR, tmp);
}

void sec_cntr_suspend(void)
{
    u32 tmp = _SOCCTRL_REG_READ(SOC_CTRL_CLK_EN_ADDR);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_SECCNTRCLKEN_MASK, 0);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_EMCLKEN_MASK, 0);
    FIELD_SET(tmp, SOC_CTRL_CLK_EN_SHLDCLKEN_MASK, 0);
    _SOCCTRL_REG_WRITE(SOC_CTRL_CLK_EN_ADDR, tmp);
}

void sec_cntr_clr_alarms(u64 channels)
{
    u32 tmp = _SEC_CNTR_REG_READ(SEC_CNTR_CONFIG_ADDR);
   
    if (channels & (SEC_CNTR_ALM_CHNL_SECURE_CLOCK_SOURCE))
    {   // specific sensor, which is part of different block
        secclk_fm_reset();
    }
    if (channels & (SEC_CNTR_ALM_CHNL_GLITCH_DET_NEGATIVE | SEC_CNTR_ALM_CHNL_GLITCH_DET_NEGATIVE_N
                    | SEC_CNTR_ALM_CHNL_GLITCH_DET_POSITIVE | SEC_CNTR_ALM_CHNL_GLITCH_DET_POSITIVE_N))
    {
        FIELD_SET(tmp, SEC_CNTR_CONFIG_GD_VCC_CLEAR_MASK, 0x1);
    }
    if (channels & (SEC_CNTR_ALM_CHNL_TEMP_SENS_HIGH | SEC_CNTR_ALM_CHNL_TEMP_SENS_HIGH_N
                    | SEC_CNTR_ALM_CHNL_TEMP_SENS_LOW | SEC_CNTR_ALM_CHNL_TEMP_SENS_LOW_N))
    {
        FIELD_SET(tmp, SEC_CNTR_CONFIG_TS_CLEAR_HIGH_MASK, 0x1);
        FIELD_SET(tmp, SEC_CNTR_CONFIG_TS_CLEAR_LOW_MASK,  0x1);
    }
    if (channels & (SEC_CNTR_ALM_CHNL_LASER_DETECTOR))
    {
        FIELD_SET(tmp, SEC_CNTR_CONFIG_LD_RESET_N_MASK, 0x0);
    }
    if (channels & (SEC_CNTR_ALM_CHNL_EM_PULSE_DETECTOR))
    {
        FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_INIT_ENA_MASK,      0x1);
        FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_BAD_INIT_DATA_MASK, 0x0);
    }

    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    os_delay_us(1);

    FIELD_SET(tmp, SEC_CNTR_CONFIG_GD_VCC_CLEAR_MASK,  0x0);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_TS_CLEAR_HIGH_MASK, 0x0);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_TS_CLEAR_LOW_MASK,  0x0);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_LD_RESET_N_MASK,    0x1);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_INIT_ENA_MASK, 0x0);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    os_delay_us(1);

    FIELD_SET(tmp, SEC_CNTR_CONFIG_EMPD_BAD_INIT_DATA_MASK, 0x1);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);

    // Clear the shield alarm status.
    _SHIELD_REG_WRITE(_SHLD1, SHIELD_STATUS_ADDR, 0x0);
    _SHIELD_REG_WRITE(_SHLD2, SHIELD_STATUS_ADDR, 0x0);

    // Clear alarm channels in the ALARM_STATUS_* registers
    u32 low = (u32)channels;
    u32 high = (u32)(channels >> 32);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_STATUS_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_STATUS_2_ADDR, high);
}

void sec_cntr_set_alarms(u64 channels)
{
    u32 low = (u32)channels;
    u32 high = (u32)(channels >> 32);

    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_SW_SET_1_ADDR, low);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_ALARM_SW_SET_2_ADDR, high);
}

u64 sec_cntr_get_alarm_memory(void)
{
    return _alarm_memory;
}

u64 sec_cntr_get_alarms(void)
{
    u32 low = _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_STATUS_1_ADDR);
    u32 high = _SEC_CNTR_REG_READ(SEC_CNTR_ALARM_STATUS_2_ADDR);

    return (((u64)high) << 32) | ((u64)low);
}

u64 sec_cntr_get_active_alarms(void)
{
    return (sec_cntr_get_alarms() & _sensors_enabled);
}

sec_cntr_life_cycle_state_t sec_cntr_lc_read(void)
{
    u32 tmp = _SEC_CNTR_REG_READ(SEC_CNTR_STATUS_ADDR);

    return FIELD_GET(SEC_CNTR_STATUS_LCSTATE_MASK, tmp);
}

void sec_cntr_lc_provision(u32 provision_val)
{
    // Define the provisioning value
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PROVISION_VAL_ADDR, provision_val);

    // Launch the provisioning sequence
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PROVISION_CTRL_ADDR, _PROVISION_CTRL_SEQ1);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PROVISION_CTRL_ADDR, _PROVISION_CTRL_SEQ2);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PROVISION_CTRL_ADDR, _PROVISION_CTRL_SEQ3);

    // Active wait till done, no need to sleep ...
    u32 tmp = 0;
    do {
        tmp = _SEC_CNTR_REG_READ(SEC_CNTR_STATUS_ADDR);
    } while (FIELD_GET(SEC_CNTR_STATUS_PRINPR_MASK, tmp));
}

void sec_cntr_precharge_en(u32 periphs, u32 prescaler, sec_cntr_precharge_src_e precharge_src)
{
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PRECHARGE_EN_ADDR, periphs);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_PRECHARGE_PRESCALER_ADDR, prescaler);

    u32 tmp = _SEC_CNTR_REG_READ(SEC_CNTR_CONFIG_ADDR);
    // enable/disable global enable flag according to periphery usage
    FIELD_SET(tmp, SEC_CNTR_CONFIG_GPEN_MASK, periphs ? 1 : 0);
    FIELD_SET(tmp, SEC_CNTR_CONFIG_PES_MASK, precharge_src);
    _SEC_CNTR_REG_WRITE(SEC_CNTR_CONFIG_ADDR, tmp);
}

void process_alarms(void)
{
    // Disable interrupt to prevent dead loop calling of interrupt
    // in case of stuck alarm source...
    sec_cntr_configure_interrupts(0);

    // Clear active alarm
    u64 active_alarms = sec_cntr_get_alarms();
    
    _alarm_memory |= active_alarms;

    LOG_WARNING("ALARM CHANNELS: %x %x", (u32)(active_alarms>>32), (u32)active_alarms);
    
    // We dont want to sec_cntr_clr_alarms(active_alarms) here
    // its better to keep the info in registers (IRQ is already off to avoid dead-loop)
}


__ISR void irq_sc_handler(void)
{   // One IRQ handler for all of irq_sc_a irq_sc_b irq_sc_c to spare some code memory.
    //
    // NOTE: This routine will be called always three times, because of parallel IRQ requests.
    //       It does not matter the IRQ is going to be disabled. The 3 IRQ request are already pending.
    
    process_alarms();
    os_alarm_isr();

}


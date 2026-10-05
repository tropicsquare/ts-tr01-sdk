/**
 * @file sec_cntr.h
 * @author Tropic Square
 * @brief Security Center
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */


#ifndef SEC_CNTR_H
#define SEC_CNTR_H

#include "type.h"
#include "bits.h"

typedef enum {
    SEC_CNTR_ALM_CHNL_PTRNG0_REDUNDANCY         = BIT64(0),       /**< PTRNG0 - Redundancy Alarm */
    SEC_CNTR_ALM_CHNL_PTRNG0                    = BIT64(1),       /**< PTRNG0 - Alarm */
    SEC_CNTR_ALM_CHNL_PTRNG1_REDUNDANCY         = BIT64(2),       /**< PTRNG1 - Redundancy Alarm */
    SEC_CNTR_ALM_CHNL_PTRNG1                    = BIT64(3),       /**< PTRNG1 - Alarm */
    SEC_CNTR_ALM_CHNL_SECURE_CLOCK_SOURCE       = BIT64(4),       /**< Secure Clock Source Alarm */
    SEC_CNTR_ALM_CHNL_SHIELD_REDUNDANCY         = BIT64(5),       /**< Shield - Redundancy Alarm */
    SEC_CNTR_ALM_CHNL_SHIELD                    = BIT64(6),       /**< Shield - Alarm */
    SEC_CNTR_ALM_CHNL_VOLT_MON_UNDERVOLTAGE     = BIT64(7),       /**< Voltage monitor - undervoltage */
    SEC_CNTR_ALM_CHNL_VOLT_MON_UNDERVOLTAGE_N   = BIT64(8),       /**< Voltage monitor - undervoltage */
    SEC_CNTR_ALM_CHNL_VOLT_MON_OVERVOLTAGE      = BIT64(9),       /**< Voltage monitor - overvoltage */
    SEC_CNTR_ALM_CHNL_VOLT_MON_OVERVOLTAGE_N    = BIT64(10),      /**< Voltage monitor - overvoltage */
    SEC_CNTR_ALM_CHNL_GLITCH_DET_NEGATIVE       = BIT64(11),      /**< Glich detector - Negative */
    SEC_CNTR_ALM_CHNL_GLITCH_DET_NEGATIVE_N     = BIT64(12),      /**< Glich detector - Negative */
    SEC_CNTR_ALM_CHNL_GLITCH_DET_POSITIVE       = BIT64(13),      /**< Glich detector - Positive */
    SEC_CNTR_ALM_CHNL_GLITCH_DET_POSITIVE_N     = BIT64(14),      /**< Glich detector - Positive */
    SEC_CNTR_ALM_CHNL_TEMP_SENS_HIGH            = BIT64(15),      /**< Temperature sensor - High */
    SEC_CNTR_ALM_CHNL_TEMP_SENS_HIGH_N          = BIT64(16),      /**< Temperature sensor - High */
    SEC_CNTR_ALM_CHNL_TEMP_SENS_LOW             = BIT64(17),      /**< Temperature sensor - Low */
    SEC_CNTR_ALM_CHNL_TEMP_SENS_LOW_N           = BIT64(18),      /**< Temperature sensor - Low */
    SEC_CNTR_ALM_CHNL_LASER_DETECTOR            = BIT64(19),      /**< Laser detector alarm */
    SEC_CNTR_ALM_CHNL_EM_PULSE_DETECTOR         = BIT64(20),      /**< EM Pulse detector alarm */
    SEC_CNTR_ALM_CHNL_CPU_MINOR                 = BIT64(21),      /**< CPU minor alert */
    SEC_CNTR_ALM_CHNL_CPU_MAJOR_INTERNAL        = BIT64(22),      /**< CPU major alert - Internal */
    SEC_CNTR_ALM_CHNL_CPU_MAJOR_BUS             = BIT64(23),      /**< CPU major alert - Bus */
    SEC_CNTR_ALM_CHNL_CPU_DOUBLE_FAULT          = BIT64(24),      /**< CPU Double fault seen */
    SEC_CNTR_ALM_CHNL_MACANDD                   = BIT64(25),      /**< MAC-and-destroy Bit flip */
    SEC_CNTR_ALM_CHNL_SCB                       = BIT64(26),      /**< Secure Channel Block Bit flip */
    SEC_CNTR_ALM_CHNL_CPB                       = BIT64(27),      /**< Command Processing Block bit flip */
    SEC_CNTR_ALM_CHNL_SPECT                     = BIT64(28),      /**< SPECT bit-flip error */
    SEC_CNTR_ALM_CHNL_FSS                       = BIT64(29),      /**< Flash Subsystem bit flip error */
    SEC_CNTR_ALM_CHNL_EDB                       = BIT64(30),      /**< EDB bit flip error */
    SEC_CNTR_ALM_CHNL_KDB                       = BIT64(31),      /**< KDB bit flip error */
    SEC_CNTR_ALM_CHNL_OTP                       = BIT64(32),      /**< OTP bit flip error */
    SEC_CNTR_ALM_CHNL_SOC_CTRL                  = BIT64(33),      /**< SoC controller bit flip error */
    SEC_CNTR_ALM_CHNL_LIFE_CYCLE                = BIT64(34)       /**< Life-cycle error */
} sec_cntr_alarm_channels_e;

typedef enum {
    SEC_CNTR_ALM_RST_CPUSS                      = (1 << 0),
    SEC_CNTR_ALM_RST_MBIST                      = (1 << 1),
    SEC_CNTR_ALM_RST_TPDI                       = (1 << 2),
    SEC_CNTR_ALM_RST_SS                         = (1 << 3),
    SEC_CNTR_ALM_RST_MAD                        = (1 << 4),
    SEC_CNTR_ALM_RST_SPECT                      = (1 << 5),
    SEC_CNTR_ALM_RST_FSS                        = (1 << 6),
    SEC_CNTR_ALM_RST_SECCNTR                    = (1 << 7),
    SEC_CNTR_ALM_RST_OTP                        = (1 << 8),
    SEC_CNTR_ALM_RST_KDB                        = (1 << 9),
    SEC_CNTR_ALM_RST_EDB                        = (1 << 10),
    SEC_CNTR_ALM_RST_SCB                        = (1 << 11),
    SEC_CNTR_ALM_RST_CPB                        = (1 << 12),
    SEC_CNTR_ALM_RST_SHLD                       = (1 << 13),
    SEC_CNTR_ALM_RST_PUF                        = (1 << 14),
    SEC_CNTR_ALM_RST_TRNG1                      = (1 << 15),
    SEC_CNTR_ALM_RST_TRNG2                      = (1 << 16),
    SEC_CNTR_ALM_RST_SCS                        = (1 << 17)
} sec_cntr_rst_peripherals_e;

typedef enum {
    SEC_CNTR_PRECHARGE_SCB_EN                   = (1 << 0),
    SEC_CNTR_PRECHARGE_CPB_EN                   = (1 << 1),
    SEC_CNTR_PRECHARGE_MACANDD_EN               = (1 << 2),
    SEC_CNTR_PRECHARGE_SPECT_EN                 = (1 << 3),
    SEC_CNTR_PRECHARGE_KDB_EN                   = (1 << 4),
    SEC_CNTR_PRECHARGE_FSS_EN                   = (1 << 5),
    SEC_CNTR_PRECHARGE_OTP_EN                   = (1 << 6),
    SEC_CNTR_PRECHARGE_MBIST_EN                 = (1 << 7)
} sec_cntr_precharge_peripherals_e;

/**
 * @brief fdsf
 * 
 */
typedef struct {
    /** @brief Interrupt enables (per Alarm channel - Mask of sec_cntr_alarm_channels_e). */
    u64             int_en;

    /** @brief Reset by active alarm (per Alarm channel - Mask of sec_cntr_alarm_channels_e). */
    u64             rst_en;

    /** @brief Reset peripherals (per peripheral - Mask of sec_cntr_rst_peripherals_e). */
    u64             rst_mask;

    /** @brief Alarms causing trigger of MBIST engine (per Alarm channel - Mask of sec_cntr_alarm_channels_e). */
    u64             mbist_en;

    /** @brief MBIST channels to be erased by alarms (Mask of mbist_chnls_t from mbist.h). */
    u32             mbist_channels;

    /** @brief Period for regular readouts of Life-cycle state by Security Center. */
    u32             refresh_wait_time;

    /** @brief Register Precharge configuration (Per-peripheral enable, mask of sec_cntr_precharge_peripherals_e). */
    u32             precharge_en;

    /** @brief Sampling of entropy prescaler. */
    u16             precharge_prescaler;

    /** @brief Interrupt handler. */
    void            (*int_handler)(void);
} sec_cntr_config_t;

typedef enum {
    LIFE_CYCLE_VIRGIN       = 0xAA,
    LIFE_CYCLE_PROVISIONED  = 0x55
} sec_cntr_life_cycle_state_t;

typedef enum {
    SEC_CNTR_PRECHARGE_SRC_PTRNG0_ARB      = 0x0,
    SEC_CNTR_PRECHARGE_SRC_PTRNG0_DNS      = 0x1,
    SEC_CNTR_PRECHARGE_SRC_PTRNG1_ARB      = 0x2,
    SEC_CNTR_PRECHARGE_SRC_PTRNG1_DNS      = 0x3
} sec_cntr_precharge_src_e;

/**
 * @brief Initialize Security Center
 *
 * @param config Security Center configutration.
 */
void sec_cntr_init(const sec_cntr_config_t *config);

/**
 * @brief Initialize Security Center app functions
 *
 * Keep and check configuration, update status only.
 */
void sec_cntr_init_app(void);


/**
 * @brief Enable set of sensors
 *
 * Usable for enabling different set of sensors which was not enabled in sec_cntr_init()
 *
 * @param channels Channels to start guarding. Shall be mask of sec_cntr_alarm_channels_e.
 *
 */
void sec_cntr_set_active_sensors(u64 channels);

/**
 * @brief Get the set of currently enabled sensors
 *
 * @returns Mask of currently enabled alarm channels (sec_cntr_alarm_channels_e).
 */
u64 sec_cntr_get_active_sensors(void) TS_CHECK_RETVAL;

/**
 * @brief Configures interrupts of Security Center
 *
 * @param channels Channels to start guarding. Shall be mask of sec_cntr_alarm_channels_e.
 */
void sec_cntr_configure_interrupts(u64 channels);

/**
 * @brief Enable clock for the Security Center.
 */
void sec_cntr_wakeup(void);

/**
 * @brief Disable clock for the Security Center.
 */
void sec_cntr_suspend(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Alarm processing
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Clear active alarms in alarm channels.
 *
 * @param channels Channels to clear. Shall be mask of sec_cntr_alarm_channels_e.
 */
void sec_cntr_clr_alarms(u64 channels);

/**
 * @brief Set active alarms in alarm channels (for testing)
 *
 * @param channels Channels to set. Shall be mask of sec_cntr_alarm_channels_e.
 */
void sec_cntr_set_alarms(u64 channels);

/**
 * @returns Mask of recorded alarms since start-up.
 */
u64 sec_cntr_get_alarm_memory(void) TS_CHECK_RETVAL;

/**
 * @returns Mask of all active alarm channels (sec_cntr_alarm_channels_e).
 */
u64 sec_cntr_get_alarms(void) TS_CHECK_RETVAL;


/**
 * @returns Mask of enabled active alarm channels (sec_cntr_alarm_channels_e).
 */
u64 sec_cntr_get_active_alarms(void) TS_CHECK_RETVAL;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Life-cycle control
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @returns Current life-cycle state of the device
 */
sec_cntr_life_cycle_state_t sec_cntr_lc_read(void) TS_CHECK_RETVAL;

/**
 * @brief Change life-cycle state of device from Virgin to Provisioned.
 *
 * @param provision_val Provisioning value to program to OTP and eFuse.
 *
 * @warning This operation involves programming of eFuse and OTP. Clock must be properly trimmed
 *          and OTP timing must be configured when this function is called. Failing to do so
 *          may result in partially trimmed device that will either:
 *              - Fail to start-up and start FW automatically
 *              - Start FW automatically, but report ALM_CHNL_LIFE_CYCLE alarm
 *              - Only partially lock TROPIC01 features available in Virgin life-cycle state.
 *                This may potentially compromise device security.
 */
void sec_cntr_lc_provision(u32 provision_val);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Precharge configuration
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Configure register pre-charge
 *
 * @param periphs Peripherals to enable precharge for (mask of sec_cntr_precharge_peripherals_e)
 * @param prescaler Number of clk_sys cycles how often to sample the entropy
 * @param precharge_src Where to sample precharge entroyp from
 */
void sec_cntr_precharge_en(u32 periphs, u32 prescaler, sec_cntr_precharge_src_e precharge_src);

#endif // ! SEC_CNTR_H


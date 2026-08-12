/**
 * @file gpo.h
 * @author Tropic Square
 * @brief GPO driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef GPO_H
#define GPO_H

#include "type.h"

/**
 * @enum gpo_e
 * @brief Enumeration of General Purpose Outputs.
 */
typedef enum {
    GPO0 = 0,  /**< GPO 0 */
    GPO1,      /**< GPO 1 */
    GPO2,      /**< GPO 2 */
    GPO3,      /**< GPO 3 */
    GPO4,      /**< GPO 4 */

    GPO_NUM // size limit only 
} gpo_e;

/**
 * @enum gpo0_src_e
 * @brief Source selectors for GPO0.
 * 
 * Each value represents a signal or clock source that can be routed to GPO0.
 */
enum gpo0_src_e {
    GPO0_SRC_NO_CONNECTION                  = 0b000000,     /** @brief No connection */
    GPO0_SRC_GPO_0_VAL                      = 0b000001,     /** @brief GPO_CTRL_1[GPO0_VAL] */
    GPO0_SRC_BD_OSC_OUT                     = 0b000010,     /** @brief Oscillator output */
    GPO0_SRC_BD_CLK_IN                      = 0b000011,     /** @brief Clock stealer input */
    GPO0_SRC_BD_CLK_STEALED                 = 0b000100,     /** @brief Clock stealer output */
    GPO0_SRC_CLK_SYS_BUS                    = 0b000101,     /** @brief System clock - Bus */
    GPO0_SRC_CLK_SYS_CPU                    = 0b000110,     /** @brief System clock - CPU */
    GPO0_SRC_CLK_SYS_MEM                    = 0b000111,     /** @brief System clock - Memories */
    GPO0_SRC_CLK_SYS_PER                    = 0b001000,     /** @brief System clock - Peripherals */
    GPO0_SRC_CLK_SYS_MBIST                  = 0b001001,     /** @brief System clock - MBIST */
    GPO0_SRC_CLK_SYS_TPDI                   = 0b001010,     /** @brief System clock - TPDI */
    GPO0_SRC_CLK_SYS_SS                     = 0b001011,     /** @brief System clock - Serial Subsystem */
    GPO0_SRC_CLK_SYS_MAD                    = 0b001100,     /** @brief System clock - MAC-and-Destroy */
    GPO0_SRC_CLK_SYS_SPECT                  = 0b001101,     /** @brief System clock - SPECT */
    GPO0_SRC_CLK_SYS_FSS                    = 0b001110,     /** @brief System clock - Flash subsystem */
    GPO0_SRC_CLK_SYS_SCNT                   = 0b001111,     /** @brief System clock - Security Center */
    GPO0_SRC_CLK_SYS_OTP                    = 0b010000,     /** @brief System clock - OTP */
    GPO0_SRC_CLK_SYS_KDB                    = 0b010001,     /** @brief System clock - Key Distribution Block */
    GPO0_SRC_CLK_SYS_EDB                    = 0b010010,     /** @brief System clock - Entropy Distribution Block */
    GPO0_SRC_CLK_SYS_SCB                    = 0b010011,     /** @brief System clock - Secure Channel Block */
    GPO0_SRC_CLK_SYS_CPB                    = 0b010100,     /** @brief System clock - Command Processing Block */
    GPO0_SRC_CLK_SYS_SHIELD                 = 0b010101,     /** @brief System clock - Shield */
    GPO0_SRC_CLK_SYS_PUF                    = 0b010110,     /** @brief System clock - PUF */
    GPO0_SRC_CLK_SYS_TRNG_1                 = 0b010111,     /** @brief System clock - TRNG 1 controller */
    GPO0_SRC_CLK_SYS_TRNG_2                 = 0b011000,     /** @brief System clock - TRNG 2   controller */
    GPO0_SRC_CLK_SYS_SCS                    = 0b011001,     /** @brief System clock - Secure Clock Source */
    GPO0_SRC_CLK_SYS_MAD_STEALED            = 0b011010,     /** @brief System clock - MAC-and-destroy stealed */
    GPO0_SRC_CLK_SYS_SPECT_STEALED          = 0b011011,     /** @brief System clock - SPECT stealed */
    GPO0_SRC_CLK_SYS_CPB_STEALED            = 0b011100,     /** @brief System clock - CPB stealed */
    GPO0_SRC_CLK_SYS_SCB_STEALED            = 0b011101,     /** @brief System clock - SCB stealed */
    GPO0_SRC_CLK_SYS_FSS_STEALED            = 0b011110,     /** @brief System clock - FSS stealed */
    GPO0_SRC_CLK_SYS_KDB_STEALED            = 0b011111,     /** @brief System clock - KDB stealed */
    GPO0_SRC_CLK_SYS_EDB_STEALED            = 0b100000,     /** @brief System clock - EDB stealed */
    GPO0_SRC_CLK_SYS_EM                     = 0b110000,     /** @brief System clock - EM detector */
    GPO0_SRC_RNG_CLK_REF_OUT_1              = 0b111110,     /** @brief PTRNG1 - Reference clock output */
    GPO0_SRC_RNG_CLK_REF_OUT_2              = 0b111111      /** @brief PTRNG2 - Reference clock output */
};

/**
 * @enum gpo1_src_e
 * @brief Source selectors for GPO1.
 */
enum gpo1_src_e {
    GPO1_SRC_NO_CONNECTION                  = 0b000000,    /** @brief No connection */
    GPO1_SRC_GPO_1_VAL                      = 0b000001,    /** @brief GPO_CTRL_1[GPO1_VAL] */
    GPO1_SRC_POR_N_VDD_1V2_0                = 0b000010,    /** @brief Power on Reset 1.2 V */
    GPO1_SRC_CPUSS_ALERT_MINOR              = 0b000011,    /** @brief CPU Minor alert */
    GPO1_SRC_CPUSS_ALERT_MAJOR_INTERNAL     = 0b000100,    /** @brief CPU Major alert - From Dual core lockstep */
    GPO1_SRC_SHIELD_ALARM_RED               = 0b000101,    /** @brief Shield Alarm Redundancy. */
    GPO1_SRC_PTRNG0_ALARM_RED               = 0b000110,    /** @brief PTRNG0 alarm - Parity Error in Register map */
    GPO1_SRC_PTRNG0_ALARM                   = 0b000111,    /** @brief PTRNG0 alarm - Analog alarm */
    GPO1_SRC_PTRNG1_ALARM_RED               = 0b001000,    /** @brief PTRNG1 alarm - Parity Error in Register map */
    GPO1_SRC_PTRNG1_ALARM                   = 0b001001,    /** @brief PTRNG1 alarm - Analog alarm */
    GPO1_SRC_SHIELD_ALARM                   = 0b001010,    /** @brief Shield alarm */
    GPO1_SRC_SC_ALARM_FM                    = 0b001011,    /** @brief Secure Clock Source - Analog alarm */
    GPO1_SRC_CPUSS_ALERT_MAJOR_BUS          = 0b001100,    /** @brief CPU Major alert - From Memory ECC */
    GPO1_SRC_LD_ALM                         = 0b001101,    /** @brief Laser detector alarm */
    GPO1_SRC_EMPD_ALARM                     = 0b001110,    /** @brief EM Pulse detector alarm */
    GPO1_SRC_CPUSS_DOUBLE_FAULT_SEEN        = 0b001111,    /** @brief CPUSS double fault seen */
    GPO1_SRC_MACANDD_BIT_FLIP               = 0b010000,    /** @brief Bit flip - MACANDD */
    GPO1_SRC_SCB_BIT_FLIP_ERR               = 0b010001,    /** @brief Bit flip - SCB */
    GPO1_SRC_CPB_BIT_FLIP_ERR               = 0b010010,    /** @brief Bit flip - CPB */
    GPO1_SRC_SPECT_BIT_FLIP_ERR             = 0b010011,    /** @brief Bit flip - SPECT */
    GPO1_SRC_FSS_BIT_FLIP_ERR               = 0b010100,    /** @brief Bit flip - FSS */
    GPO1_SRC_EDB_BIT_FLIP_ERR               = 0b010101,    /** @brief Bit flip - EDB */
    GPO1_SRC_KDB_BIT_FLIP_ERR               = 0b010110,    /** @brief Bit flip - KDB */
    GPO1_SRC_OTP_BIT_FLIP_ERR               = 0b010111,    /** @brief Bit flip - OTP */
    GPO1_SRC_SOCCTRL_BIT_FLIP_ERR           = 0b011000     /** @brief Bit flip - SOCCTRL */
};

/**
 * @enum gpo2_src_e
 * @brief Source selectors for GPO2.
 */
enum gpo2_src_e {
    GPO2_SRC_NO_CONNECTION                  = 0b000000,     /** @brief No connection */
    GPO2_SRC_GPO_2_VAL                      = 0b000001,     /** @brief GPO_CTRL_1[GPO2_VAL] */
    GPO2_SRC_MACANDD_RBUS_REQ               = 0b000011,     /** @brief MAC-and-Destroy RBUS request */
    GPO2_SRC_MACANDD_KBUS_REQ               = 0b000100,     /** @brief MAC-and-Destroy KBUS request */
    GPO2_SRC_MACANDD_FLASH_REQ              = 0b000101,     /** @brief MAC-and-Destroy Flash Interface request */
    GPO2_SRC_MACANDD_CPB_REQ                = 0b000110,     /** @brief MAC-and-Destroy CPB request */
    GPO2_SRC_MON_VCC_LOW_FLAG               = 0b001010,     /** @brief Voltage Monitor - Low voltage */
    GPO2_SRC_MON_VCC_HIGH_FLAG_NOT          = 0b001011,     /** @brief Voltage Monitor - High voltage Inverted */
    GPO2_SRC_GD_VCC_FLAG_NEG                = 0b001100,     /** @brief Glitch Detector - Negative glitch */
    GPO2_SRC_GD_VCC_FLAG_POS_NOT            = 0b001101,     /** @brief Glitch Detector - Positive glitch Inverted */
    GPO2_SRC_TS_FLAG_HIGH                   = 0b001110,     /** @brief Temperature Sensor - High Temperature */
    GPO2_SRC_TS_FLAG_LOW_NOT                = 0b001111,     /** @brief Temperature Sensor - Low Temperature Inverted */
    GPO2_SRC_RNG_ARBITER_OUT_1              = 0b010000,     /** @brief PTRNG0 - Analog Arbiter Output */
    GPO2_SRC_RNG_ARBITER_OUT_2              = 0b010001,     /** @brief PTRNG1 - Analog Arbiter Output */
    GPO2_SRC_PUF_REQ                        = 0b010010,     /** @brief PUF direct output request */
    GPO2_SRC_PTRNG_REQ                      = 0b010011,     /** @brief PTRNG direct output request */
    GPO2_SRC_SCB_KBUS_REQ                   = 0b010100,     /** @brief Secure Channel KBUS Request */
    GPO2_SRC_SPECT_KBUS_REQ                 = 0b010101,     /** @brief SPECT KBUS Request */
    GPO2_SRC_FSS_KBUS_REQ                   = 0b010110,     /** @brief Flash Subsystem KBUS Request */
    GPO2_SRC_SCB_RBUS_REQ                   = 0b010111,     /** @brief Secure Channel RBUS Request */
    GPO2_SRC_SPECT_RBUS_REQ                 = 0b011000,     /** @brief SPECT RBUS Request */
    GPO2_SRC_FSS_RBUS_REQ                   = 0b011001,     /** @brief Flash Subsystem RBUS Request */
    GPO2_SRC_OTP_DMI_REQ_0                  = 0b011010,     /** @brief OTP DMI Request (From Security Center) */
    GPO2_SRC_OTP_DMI_REQ_1                  = 0b011011,     /** @brief OTP DMI Request (From KDB) */
    GPO2_SRC_SCB_CI_REQ                     = 0b011100,     /** @brief Secure Channel - Command Input Interface request */
    GPO2_SRC_SPECT_COI_REQ                  = 0b011101,     /** @brief SPECT - Command Output Interface request */
    GPO2_SRC_MACANDD_COI_DATA_REQ           = 0b011110      /** @brief MACANDD - Command Output Interface request */
};

/**
 * @enum gpo3_src_e
 * @brief Source selectors for GPO3.
 */
enum gpo3_src_e {
    GPO3_SRC_NO_CONNECTION                  = 0b000000,     /** @brief No connection */
    GPO3_SRC_GPO_3_VAL                      = 0b000001,     /** @brief GPO_CTRL_1[GPO3_VAL] */
    GPO3_SRC_MACANDD_RBUS_GNT               = 0b000011,     /** @brief MAC-and-Destroy RBUS grant */
    GPO3_SRC_MACANDD_KBUS_GNT               = 0b000100,     /** @brief MAC-and-Destroy KBUS grant */
    GPO3_SRC_MACANDD_FLASH_GNT              = 0b000101,     /** @brief MAC-and-Destroy Flash Interface grant */
    GPO3_SRC_MACANDD_CPB_GNT                = 0b000110,     /** @brief MAC-and-Destroy CPB grant */
    GPO3_SRC_MON_VCC_LOW_FLAG_NOT           = 0b001010,     /** @brief Voltage Monitor - Low voltage Inverted */
    GPO3_SRC_MON_VCC_HIGH_FLAG              = 0b001011,     /** @brief Voltage Monitor - High voltage */
    GPO3_SRC_GD_VCC_FLAG_NEG_NOT            = 0b001100,     /** @brief Glitch Detector - Negative glitch Inverted */
    GPO3_SRC_GD_VCC_FLAG_POS                = 0b001101,     /** @brief Glitch Detector - Positive glitch */
    GPO3_SRC_TS_FLAG_HIGH_NOT               = 0b001110,     /** @brief Temperature Sensor - High Temperature Inverted */
    GPO3_SRC_TS_FLAG_LOW                    = 0b001111,     /** @brief Temperature Sensor - Low Temperature */
    GPO3_SRC_PTRNG0_ARB_OUTPUT              = 0b010000,     /** @brief PTRNG0 - Sampled Arbiter Output */
    GPO3_SRC_PTRNG1_ARB_OUTPUT              = 0b010001,     /** @brief PTRNG1 - Sampled Arbiter Output */
    GPO3_SRC_PUF_GNT                        = 0b010010,     /** @brief PUF direct output request */
    GPO3_SRC_PTRNG_GNT                      = 0b010011,     /** @brief PTRNG direct output request */
    GPO3_SRC_SCB_KBUS_GNT                   = 0b010100,     /** @brief Secure Channel KBUS Grant */
    GPO3_SRC_SPECT_KBUS_GNT                 = 0b010101,     /** @brief SPECT KBUS Grant */
    GPO3_SRC_FSS_KBUS_GNT                   = 0b010110,     /** @brief Flash Subsystem KBUS Grant */
    GPO3_SRC_SCB_RBUS_GNT                   = 0b010111,     /** @brief Secure Channel RBUS Grant */
    GPO3_SRC_SPECT_RBUS_GNT                 = 0b011000,     /** @brief SPECT RBUS Grant */
    GPO3_SRC_FSS_RBUS_GNT                   = 0b011001,     /** @brief Flash Subsystem RBUS Grant */
    GPO3_SRC_OTP_DMI_GNT_0                  = 0b011010,     /** @brief OTP DMI Grant (From Security Center) */
    GPO3_SRC_OTP_DMI_GNT_1                  = 0b011011,     /** @brief OTP DMI Grant (From KDB) */
    GPO3_SRC_SCB_CI_GNT                     = 0b011100,     /** @brief Secure Channel - Command Input Interface grant */
    GPO3_SRC_SPECT_COI_GNT                  = 0b011101,     /** @brief SPECT - Command Output Interface grant */
    GPO3_SRC_MACANDD_COI_DATA_GNT           = 0b011110,     /** @brief MACANDD - Command Output Interface grant */
    GPO3_SRC_CPUSS_INT                      = 0b100000,     /** @brief Interrupt Channel selected by GPO_CTRL_2[GPO_INT_INDEX] */
    GPO3_SRC_PUF_OUT                        = 0b100001,     /** @brief PUF value bit selected by GPO_CTRL_2[PUF_INT_INDEX] */
    GPO3_SRC_PTRNG0_DNS_OUTPUT              = 0b100010,     /** @brief PTRNG0 - Digital Noise Source */
    GPO3_SRC_PTRNG1_DNS_OUTPUT              = 0b100011,     /** @brief PTRNG1 - Digital Noise Source */
    GPO3_SRC_BD_DIG_OUT                     = 0b100100,     /** @brief Bedrock Analog (PM55) - Digital observation */
    GPO3_SRC_ALARM_STATUS                   = 0b100101      /** @brief Active alarm on alarm channel selected by GPO_CTRL_2[GPO_ALARM_INDEX]. */
};

/**
 * @brief Initializes the GPO system.
 * 
 * This function must be called before using any other GPO-related functions.
 */
void gpo_init(void);

/**
 * @brief Sets the source of the specified GPO pin.
 * 
 * @param gpo The GPO pin to configure (GPO0 - GPO3).
 * @param src The source signal to assign to the GPO.
 */
void gpo_set_src(gpo_e gpo, u32 src);

/**
 * @brief Turns on the specified GPO pin.
 * 
 * @param gpo The GPO pin to set high.
 */
void gpo_on(gpo_e gpo);

/**
 * @brief Turns off the specified GPO pin.
 * 
 * @param gpo The GPO pin to set low.
 */
void gpo_off(gpo_e gpo);

/**
 * @brief Read out state of the specified GPO pin.
 * 
 * @param gpo The GPO pin to read.
 * @return true if state is active (HI), false otherwise.
 */
bool gpo_state(gpo_e gpo);

/**
 * @brief Toggles the state of the specified GPO pin.
 * 
 * @param gpo The GPO pin to toggle.
 */
void gpo_toggle(gpo_e gpo);


/**
 * @enum gpo4_mode_e
 * @brief Operating modes for GPO4.
 *
 * GPO4 is the only one which is user accessible in application. 
 * Other GPOs works only in virgin state before provisioning.
 */
enum gpo4_mode_e {
    GPO4_MODE_INTERRUPT   = 0b0111, /**< GPO4 functions as interrupt output, the default value */
    GPO4_MODE_ALWAYS_HIGH = 0b0110, /**< GPO4 remains always high */
    GPO4_MODE_ALWAYS_LOW  = 0b0101  /**< GPO4 remains always low */
};

/**
 * @brief Sets the operating mode for GPO4.
 * 
 * @param mode One of the values from gpo4_mode_e.
 */
void gpo4_set_mode(u32 mode);

/**
 * @brief Sets GPO4 high (logic 1).
 */
void gpo4_on(void);

/**
 * @brief Sets GPO4 low (logic 0).
 */
void gpo4_off(void);

/**
 * @brief Sets the GPO interrupt signal.
 * 
 * This is used in GPO4 interrupt mode to trigger an interrupt.
 */
void gpo_int_set(void);

/**
 * @brief Clears the GPO interrupt signal.
 * 
 * Used to deassert the interrupt signal in GPO4 interrupt mode.
 */
void gpo_int_clr(void);


#endif // ~ GPO_H


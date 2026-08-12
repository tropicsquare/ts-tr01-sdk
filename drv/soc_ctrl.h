/**
 * @file soc_ctrl.h
 * @author Tropic Square
 * @brief SoC control functions header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SOC_CTRL_H
#define SOC_CTRL_H

#include "type.h"
#include "tassic_defs.h"
#include "soc_ctrl_regs.h"

/**
 * @brief All peripheral clocks.
 * 
 */
typedef enum {

    SOC_CTRL_CLK_BUS               = SOC_CTRL_CLK_EN_BUSCLKEN_MASK,
    SOC_CTRL_CLK_CPU_CORE          = SOC_CTRL_CLK_EN_CPUCLKEN_MASK,
    SOC_CTRL_CLK_CPU_MEM           = SOC_CTRL_CLK_EN_MEMCLKEN_MASK,
    SOC_CTRL_CLK_CPU_PERIPHERALS   = SOC_CTRL_CLK_EN_PERCLKEN_MASK,
    SOC_CTRL_CLK_MBIST             = SOC_CTRL_CLK_EN_MBISTCLKEN_MASK,
    SOC_CTRL_CLK_TPDI              = SOC_CTRL_CLK_EN_TPDICLKEN_MASK,
    SOC_CTRL_CLK_SERIAL_SS         = SOC_CTRL_CLK_EN_SSCLKEN_MASK,
    SOC_CTRL_CLK_MAC_AND_D         = SOC_CTRL_CLK_EN_MADCLKEN_MASK,
    SOC_CTRL_CLK_SPECT             = SOC_CTRL_CLK_EN_SPECTCLKEN_MASK,
    SOC_CTRL_CLK_FSS               = SOC_CTRL_CLK_EN_FSSCLKEN_MASK,
    SOC_CTRL_CLK_SEC_CTR           = SOC_CTRL_CLK_EN_SECCNTRCLKEN_MASK,
    SOC_CTRL_CLK_OTP               = SOC_CTRL_CLK_EN_OTPCLKEN_MASK,
    SOC_CTRL_CLK_KDB               = SOC_CTRL_CLK_EN_KDBCLKEN_MASK,
    SOC_CTRL_CLK_EDB               = SOC_CTRL_CLK_EN_EDBCLKEN_MASK,
    SOC_CTRL_CLK_SCB               = SOC_CTRL_CLK_EN_SCBCLKEN_MASK,
    SOC_CTRL_CLK_CPB               = SOC_CTRL_CLK_EN_CPBCLKEN_MASK,
    SOC_CTRL_CLK_SHIELD            = SOC_CTRL_CLK_EN_SHLDCLKEN_MASK,
    SOC_CTRL_CLK_PUF               = SOC_CTRL_CLK_EN_PUFCLKEN_MASK,
    SOC_CTRL_CLK_TRNG1             = SOC_CTRL_CLK_EN_TRNG1CLKEN_MASK,
    SOC_CTRL_CLK_TRNG2             = SOC_CTRL_CLK_EN_TRNG2CLKEN_MASK,
    SOC_CTRL_CLK_SCS               = SOC_CTRL_CLK_EN_SCSCLKEN_MASK,
    SOC_CTRL_CLK_EMP_DETECTOR      = SOC_CTRL_CLK_EN_EMCLKEN_MASK,

    SOC_CTRL_CLK_CPU_SUBSET        = (SOC_CTRL_CLK_BUS | SOC_CTRL_CLK_CPU_CORE | SOC_CTRL_CLK_CPU_MEM |SOC_CTRL_CLK_CPU_PERIPHERALS),
    SOC_CTRL_CLK_TPDI_SUBSET       = (SOC_CTRL_CLK_BUS | SOC_CTRL_CLK_TPDI ),
    SOC_CTRL_CLK_TPDI_GROUP        = (SOC_CTRL_CLK_TPDI_SUBSET | SOC_CTRL_CLK_CPU_SUBSET),
    SOC_CTRL_CLK_PUF_SUBSET       = (SOC_CTRL_CLK_PUF),
    SOC_CTRL_CLK_PTRNG_SUBSET     = (SOC_CTRL_CLK_TRNG1 | SOC_CTRL_CLK_TRNG2),
    SOC_CTRL_CLK_FLASH_SUBSET     = (SOC_CTRL_CLK_FSS | SOC_CTRL_CLK_EDB | SOC_CTRL_CLK_KDB),

    SOC_CTRL_CLK_ALL               = GENMASK(21,0)
} soc_ctrl_periph_clk_en_e;

/**
 * Type for multiple soc_ctrl_periph_clk_en_e selection.
 */
typedef u32 soc_ctrl_periph_clk_en_t;

typedef enum {

    SOC_CTRL_SCLK_MAC_AND_D = SOC_CTRL_CLK_SRC_MADCLKSRC_MASK,
    SOC_CTRL_SCLK_SPECT     = SOC_CTRL_CLK_SRC_SPECTCLKSRC_MASK,
    SOC_CTRL_SCLK_FSS       = SOC_CTRL_CLK_SRC_FSSCLKSRC_MASK,
    SOC_CTRL_SCLK_KDB       = SOC_CTRL_CLK_SRC_KDBCLKSRC_MASK,
    SOC_CTRL_SCLK_EDB       = SOC_CTRL_CLK_SRC_EDBCLKSRC_MASK,
    SOC_CTRL_SCLK_SCB       = SOC_CTRL_CLK_SRC_SCBCLKSRC_MASK,
    SOC_CTRL_SCLK_CPB       = SOC_CTRL_CLK_SRC_CPBCLKSRC_MASK
} soc_ctrl_periph_sclk_e;

/**
 * Type for multiple soc_ctrl_periph_sclk_e selection.
 */
typedef u32 soc_ctrl_periph_sclk_t;

/**
 * System wide basic setup.
 */
void soc_ctrl_init(void);

/**
 * Enable clock for any peripheral.
 *
 * @param[in] peripherals target peripheral
 *
 * @note When enabling clocks of multiple peripherals,
 *       please use the bitwise OR "|" instead of the + operator.
 */
void soc_ctrl_clk_en (soc_ctrl_periph_clk_en_t peripherals);

void soc_ctrl_clk_dis(soc_ctrl_periph_clk_en_t peripherals);
#define soc_ctrl_clk_off soc_ctrl_clk_dis // backward compatibility define, to be removed 
/**
 * Enable secure clocks for supported peripherals.
 *
 * @param[in] peripherals target peripheral
 *
 * @note When enabling secure clocks of multiple peripherals,
 *       please use the bitwise OR "|" instead of the + operator.
 */
void soc_ctrl_sclk_en (soc_ctrl_periph_sclk_t peripherals);

void soc_ctrl_sclk_dis(soc_ctrl_periph_sclk_t peripherals);
#define soc_ctrl_sclk_off soc_ctrl_sclk_dis // backward compatibility define, to be removed

/**
 * Divide clock by two.
 */
void soc_ctrl_clk_div(void);

/**
 * @name Enable secure clocks for supported peripherals
 */
///@{
void soc_ctrl_sclk_mac_and_d_clk_en (void);

void soc_ctrl_sclk_spect_clk_en (void);

void soc_ctrl_sclk_cpb_clk_en (void);
///@}

/**
 * @brief Prepare for sleep mode
 */
void soc_ctrl_sleep_prepare(void);

/**
 * @brief Enter sleep mode by disabling oscillator
 */
void soc_ctrl_sleep_enter(void);

/**
 * @brief Re-enable functions switched off by soc_ctrl_sleep_enter()
 */
void soc_ctrl_sleep_leave(void);

/**
 * @brief Does sleep prepare, sleep enter and sleep leave when done
 */
void soc_ctrl_sleep_mode(void);

void soc_ctrl_deep_sleep(void);

/**
 * @brief Power on LDOs 
 * @param[in] bits according to SOC_CTRL_PWR_ENA_*
 */
void soc_ctrl_pwr_on(u32 bits);

/**
 * @brief Power off LDOs 
 * @param[in] bits according to SOC_CTRL_PWR_ENA_*
 */
void soc_ctrl_pwr_off(u32 bits);

/**
 * @brief Reset periphery
 * @param[in] bits according to SOC_CTRL_UTRESET_*
 */
void soc_ctrl_reset(u32 bits);


#endif  // ! SOC_CTRL_H

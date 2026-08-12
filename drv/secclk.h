/**
 * @file secclk.h
 * @author Tropic Square
 * @brief Secure Clock driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SECCLK_H
#define SECCLK_H

#include "type.h"
#include "soc_ctrl.h"


typedef enum {
    SECCLK_CFG_INTENSITY_MIN = 0,
    SECCLK_CFG_INTENSITY_1,
    SECCLK_CFG_INTENSITY_2,
    SECCLK_CFG_INTENSITY_3,
    SECCLK_CFG_INTENSITY_4,
    SECCLK_CFG_INTENSITY_5,
    SECCLK_CFG_INTENSITY_6,
    SECCLK_CFG_INTENSITY_MAX = 7,
    
    SECCLK_CFG_INTENSITY_NUM
} secclk_cfg_intensity_e;

/**
 * @brief Set trimming based on manufacturing data from NVR1.
 * Also frequency monitor will be enabled.
 *
 * @param[in] trim Value to set in SECURE_CLOCK_CG_CONFIG register
 */
void secclk_trim(u32 trim);

/**
 * @brief Start the secure clock and configure intensity of clocks security.
 * @param[in] intensity Desired intensity.
 */
void secclk_init(secclk_cfg_intensity_e intensity);

/**
 * @brief Reset frequency monitor.
 */
void secclk_fm_reset(void);

/**
 * @brief Select periphery to use the secure clock subsystem.
 * @param[in] peripherals Selected peripheries.
 */
inline void secclk_enable(soc_ctrl_periph_sclk_t peripherals) 
{
    soc_ctrl_sclk_en(peripherals);
}

#endif // ! SECCLK_H


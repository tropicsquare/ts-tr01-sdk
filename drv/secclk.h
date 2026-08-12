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
    SECCLK_STEALING_INTENSITY_MIN = 0,
    SECCLK_STEALING_INTENSITY_1,
    SECCLK_STEALING_INTENSITY_2,
    SECCLK_STEALING_INTENSITY_3,
    SECCLK_STEALING_INTENSITY_4,
    SECCLK_STEALING_INTENSITY_5,
    SECCLK_STEALING_INTENSITY_6,
    SECCLK_STEALING_INTENSITY_MAX = 7,
    
    SECCLK_STEALING_INTENSITY_NUM
} secclk_stealing_intensity_e;

/**
 * @brief Set trimming based on manufacturing data from NVR1.
 * @param[in] trim Value to set in SECURE_CLOCK_CG_CONFIG register
 */
void secclk_trim(u32 trim);

/**
 * @brief Start the secure clock and configure intensity of clocks stealing.
 * @param[in] intensity Desired intensity of stealing.
 */
void secclk_init(secclk_stealing_intensity_e intensity);

/**
 * @brief Select periphery to use the secure clock subsystem.
 * @param[in] peripherals  Selected periphery.
 */
inline void secclk_enable(soc_ctrl_periph_stealed_clk_t peripherals) 
{
    soc_ctrl_stealed_clk_en(peripherals);
}

#endif // ! SECCLK_H


/**
 * @file cpuss_defs.h
 * @author Tropic Square
 * @brief CPU subsystem definitions and macros.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef __CPUSS_DEFS
#define __CPUSS_DEFS

#define IROM_BASE_ADDRESS               0x00000000
#define IROM_SIZE                       16384

#define IRAM_BASE_ADDRESS               0x00100000
#define IRAM_SIZE                       24576

#define DRAM_BASE_ADDRESS               0x00200000
#define DRAM_SIZE                       16384

#define IRQ_CTRL_BASE_ADDRESS           0x01000000
#define IRQ_CTRL_SIZE                   4096

#define TIMER_1_BASE_ADDRESS            0x01100000
#define TIMER_1_SIZE                    4096

#define TIMER_2_BASE_ADDRESS            0x01200000
#define TIMER_2_SIZE                    4096

#define AHB_PERIPHERALS_BASE_ADDRESS    0x02000000
#define AHB_PERIPHERALS_SIZE            4292870144

#endif

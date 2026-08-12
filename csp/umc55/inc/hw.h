/**
 * @file hw.h
 * @author Tropic Square
 * @brief HW defines
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef HW_H
#define HW_H

#if TS_SIMULATION_BUILD == 1
  /** @brief simulation runs exactly 33752995 Hz. */
  #define HW_CLOCK_MHZ (34)
#else
  #define HW_CLOCK_MHZ (70)
#endif 

#define HW_CLOCK_HZ (HW_CLOCK_MHZ * 1000000)

#endif // ! HW_H


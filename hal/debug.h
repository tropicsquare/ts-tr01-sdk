/**
 * @file debug.h
 * @author Tropic Square
 * @brief Debug messages output header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "type.h"

#if (TS_SIMULATION_BUILD == 1)
  #define DEBUG_FTC 1
#endif // TS_SIMULATION_BUILD

// If TS_LOG_SPI is undefined, disable TS_LOG_SPI
#ifndef TS_LOG_SPI
  #define TS_LOG_SPI 0
#endif

/**
 * @brief Read data from debug buffer to destination buffer.
 * @param[in] data The destination where to copy data
 * @param[in] limit Maximal number of bytes copied
 * @returns Number of bytes actually read. 
 */
size_t debug_fetch(u8 *data, size_t limit);

/**
 * @brief Flush data exceed limit.
 * @param[in] limit Maximal number of bytes to keep in buffer.
 */
void debug_crop(size_t limit);

/**
 * @brief Put one character (one byte) to buffer.
 * @param[in] ch Ascii character.
 */
void debug_put_char(ascii ch);

/**
 * @brief Flush cached data.
 * @note: Usable only when working with FTC output.
 */
void debug_flush(void);

#endif // ! DEBUG_H

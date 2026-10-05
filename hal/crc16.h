/**
 * @file crc16.h
 * @author Tropic Square
 * @brief CRC checksum computation header file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef CRC16_H
#define CRC16_H

#include "type.h"

/**
 * @name  CRC parameterization
 */
///@{

/** @brief Generator polynomial value used. */
#define CRC16_POLYNOMIAL      0x8005

/** @brief Used to initialize the crc value. */
#define CRC16_INITIAL_VAL     0x0000

/** @brief The final XOR value is xored to the final CRC value before being returned.
 *         This is done after the 'Result reflected' step. */
#define CRC16_FINAL_XOR_VALUE 0x0000

///@}

u16 crc16_byte(u8 data, u16 crc) TS_CHECK_RETVAL;

/**
 * Compute the CRC16 checksum for an array
 *
 * @param[in] data array of bytes
 * @param[in] len number of bytes
 * @return the checksum
 */
u16 crc16(const u8 *data, size_t len) TS_CHECK_RETVAL;

#endif // ! CRC16_H


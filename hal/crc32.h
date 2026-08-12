/**
 * @file crc32.h
 * @author Tropic Square
 * @brief CRC32 computation header file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef CRC32_H
#define CRC32_H

#include "type.h"

/**
 * @brief Compute the CRC32 checksum for an array
 *
 * @param[in] data array of bytes
 * @param[in] len number of bytes
 * @return the checksum
 */
u32 crc32(const u8 *data, size_t len);

#endif // ! CRC32_H


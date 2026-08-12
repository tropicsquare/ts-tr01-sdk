/**
 * @file util.h
 * @author Tropic Square
 * @brief Some universal utility header file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef UTIL_H
#define UTIL_H

#include "type.h"

/**
 * @brief Perform a constant-time memory comparison.
 *
 * Compares two memory blocks byte-by-byte in constant time,
 * meaning the execution time does not depend on the data values.
 * This is important for cryptographic operations to avoid timing attacks.
 *
 * @param a Pointer to the first memory block.
 * @param b Pointer to the second memory block.
 * @param size Number of bytes to compare.
 * @return 0 if the memory blocks are equal, non-zero otherwise.
 */
int ct_memcmp(const void *a, const void *b, size_t size);


/**
 * @brief Perform memory clear.
 *
 * Fast variant for memory clear.
 * The memset() implementation in RiscV32 is not effective, it writes byte by byte.
 *
 * @param dest Memory destination, must be 32bit alligned.
 * @param size Number of bytes to clear, must be 32bit alligned.
 *
 */
void memclear32(u32 *dest, u32 size);

#endif // ! UTIL_H

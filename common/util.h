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
int ct_memcmp(const void *a, const void *b, size_t size) TS_CHECK_RETVAL;


/**
 * @brief Overwrite a memory region with zeros.
 *
 * Unlike memset(), this function is guaranteed not to be optimized out by the
 * compiler. The write is performed through a volatile-qualified pointer.
 *
 * Use it where the zero value itself is part of the contract - i.e. to put a
 * structure into a known state which the code around it relies on. To destroy
 * data whose value does not matter afterwards, prefer memerase_safe().
 *
 * @note Clears 32bit word at a time, an unaligned start and end of the region are
 * cleared byte-by-byte. So there is no alignment requirement on @p dest.
 * @warning @p dest is not checked for NULL.
 *
 * @param dest Pointer to the memory region to clear.
 * @param size Number of bytes to overwrite with zeros.
 */
void memzero_safe(void *dest, size_t size);

/**
 * @brief Securely erase a memory region by overwriting it with random data.
 *
 * Unlike memset(), this function:
 *  1. Overwrites the buffer with pseudo-random "rubbish" data from the PRNG
 *     (prng.h) instead of a fixed value
 *  2. Is guaranteed not to be optimized out by the compiler. The write is
 *     performed through a volatile-qualified pointer
 *
 * @note Until the PRNG is seeded (see prng_seed()) the region is overwritten with
 * zeros, so the erase itself never fails and never needs the PRNG to be ready. The
 * fill value is therefore not guaranteed - use memzero_safe() when it matters.
 * @note Erases byte-by-byte, so there is no alignment requirement on @p dest.
 * @warning @p dest is not checked for NULL.
 *
 * @param dest Pointer to the memory region to erase.
 * @param size Number of bytes to overwrite.
 */
void memerase_safe(void *dest, size_t size);

#endif // ! UTIL_H

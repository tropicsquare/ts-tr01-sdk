/**
 * @file scramble.h
 * @brief Scrambling utility for OTP and FLASH scrambling manipulation.
 * @author Tropic Square
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SCRAMBLE_H
#define SCRAMBLE_H

#include "type.h"

/**
 * @brief Init scrambling sequence.
 *
 * The init value is "not scrambled" just row of numbers.
 *
 * @param[in]  n        Number of valid items in sequence (bytes).
 * @param[out] sequence The destination sequence.
 */
void scramble_init(u8 *sequence, size_t n);

/**
 * @brief Shuffle the sequence.
 *
 * @param[in]     n        Number of valid items in sequence (bytes).
 * @param[in]     seed     Pseudo random data used for scrambling.
 * @param[in,out] sequence The sequence for shuffling.
 *
 * @note Number of seed bytes must be equal or higher than *n*.
 */
void scramble_shuffle(u8 *sequence, size_t n, const u8 *seed);


/**
 * @brief Get the final scrambling value.
 *
 * @param[in] sequence Reordered row of bytes for scrambling setup.
 * @param[in] n Number of valid items in sequence (bytes).
 * @return The value suitable for register content.
 */
u32 scramble_value(const u8 *sequence, size_t n) TS_CHECK_RETVAL;

/**
 * @brief Get the final scrambling value in reversed order of nibbles.
 *
 * This is backward compatibility implementation. 
 *
 * @param[in] sequence Reordered row of bytes for scrambling setup.
 * @param[in] n Number of valid items in sequence (bytes).
 * @return The value suitable for register content.
 */
u32 scramble_value_reversed(const u8 *sequence, size_t n) TS_CHECK_RETVAL;

#endif // ! SCRAMBLE_H


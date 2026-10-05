/**
 * @file prng.h
 * @author Tropic Square
 * @brief Simple fast pseudo-random number generator (PRNG) interface.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef PRNG_H
#define PRNG_H

#include "type.h"

/**
 * @brief Initializes the pseudo-random number generator with a given seed.
 *
 * This function sets the initial seed value for the PRNG. Use this to ensure
 * repeatable sequences of random values if the same seed is used.
 *
 * @param seed A 32-bit unsigned integer to seed the generator.
 */
void prng_seed(u32 seed);

/**
 * @brief Reads the next pseudo-random 32-bit unsigned integer.
 *
 * The sequence depends on the last seed value set by `prng_seed()`. The generator
 * is a plain LCG, so the values are not suitable for any cryptographic purpose.
 *
 * @param[out] value Buffer for the value. Written only on success.
 * @return `TS_TRUE` on success, `TS_FALSE` when @p value is NULL or when the PRNG
 *         has not been seeded yet.
 */
ts_bool prng_read(u32 *value) TS_CHECK_RETVAL;

#endif // ! PRNG_H

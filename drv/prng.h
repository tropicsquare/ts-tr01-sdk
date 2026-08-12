/**
 * @file prng.h
 * @author Tropic Square
 * @brief Simple fast pseudo-random number generator (PRNG) interface.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

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
 * @brief Retrieves the next pseudo-random 32-bit unsigned integer.
 *
 * This function generates and returns the next value in the pseudo-random sequence.
 * The sequence depends on the last seed value set by `prng_seed()`.
 *
 * @return A pseudo-random 32-bit unsigned integer.
 */
u32 prng_get_value_insecure(void);

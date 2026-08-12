/**
 * @file scramble.c
 * @brief Scrambling utility for OTP and FLASH scrambling manipulation.
 * @author Tropic Square
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "scramble.h"

#define _SCRAMBLE_BIT_SIZE        4
#define _SCRAMBLE_BIT_MASK        (0x0F)
#define _SCRAMBLE_WORD_NIBBLES    (32 / _SCRAMBLE_BIT_SIZE)

static void _swap(u8 *sequence, size_t a, size_t b)
{
    u8 tmp = sequence[a];
    sequence[a] = sequence[b];
    sequence[b] = tmp;
}


void scramble_init(u8 *sequence, size_t n)
{
    size_t i;

    OS_SANITY_NULL(sequence);

    for (i=0; i<n; i++)
    {
        sequence[i] = (u8)i;
    }
}

void scramble_shuffle(u8 *sequence, size_t n, const u8 *seed)
{   // shuffle order on <n> elements in <sequence> based on <seed>
    // <seed> sequence of per-device unique pseudo-random numbers (based on PUF)
    size_t i;

    OS_SANITY_NULL(sequence);
    OS_SANITY_NULL(seed);
    
    for (i = n - 1; i > 0; i--) 
    {
        _swap(sequence, i, seed[i] % (i+1));
    }
}

u32 scramble_value(const u8 *sequence, size_t n)
{   // build scrambling value from sequence of numbers
    u32 value = 0;
    size_t i;
   
    OS_SANITY_NULL(sequence);
    OS_ASSERT(n <= _SCRAMBLE_WORD_NIBBLES);

    for (i=0; i<n; i++)
    {
        value <<= _SCRAMBLE_BIT_SIZE;
        value |= (sequence[i] & _SCRAMBLE_BIT_MASK);
    }
    return (value);
}



/**
 * @file prng.c
 * @author Tropic Square
 * @brief Simple fast pseudo-random number generator (PRNG) interface.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "prng.h"

static u32 _seed;
static bool _seed_done = false;

void prng_seed(u32 seed)
{
    _seed = seed;
    _seed_done = true;
}

ts_bool prng_read(u32 *value)
{
    if (value == NULL)
    {   // nowhere to deliver the value
        return TS_FALSE;
    }

    if (_seed_done != true)
    {   // no sequence to continue yet, the caller decides how bad that is
        return TS_FALSE;
    }

    _seed = _seed * 1664525U + 1013904223U;
    // NOTE: The multiplier 1664525 and increment 1013904223 are classic values used by 
    //       many old-school PRNGs (like the one in ANSI C).
    *value = _seed;
    return TS_TRUE;
}

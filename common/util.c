/**
 * @file util.c
 * @author Tropic Square
 * @brief Some universal utility source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "util.h"
#include "os.h"
#include "prng.h"

int ct_memcmp(const void *a, const void *b, size_t size) 
{
    const u8 *p1 = (const u8 *)a;
    const u8 *p2 = (const u8 *)b;
    u8 result = 0;

    for (size_t i = 0; i < size; i++) 
    {
        result |= p1[i] ^ p2[i];
    }

    // Return 0 if blocks are equal, non-zero otherwise.
    return result;
}

void memzero_safe(void *dest, size_t size)
{
    // Write through a volatile-qualified pointer: accesses to volatile objects
    // are observable side effects, so the compiler is not permitted to elide
    // these stores even if 'dest' is never read again (dead-store elimination).
    volatile u8 *p = (volatile u8 *)dest;

    // Byte prologue: clear up to the first 32bit boundary, so the bulk below can
    // use word writes even when 'dest' is not aligned.
    while ((size != 0) && (((uintptr_t)p & (sizeof(u32) - 1)) != 0))
    {
        *p = 0;
        p++;
        size--;
    }

    // Bulk: one word write per 4 bytes, so 4x less stores than a byte loop.
    // The prologue above guarantees the 32bit alignment, the cast goes through
    // uintptr_t because a direct pointer cast trips -Wcast-align=strict.
    volatile u32 *pw = (volatile u32 *)(uintptr_t)p;
    while (size >= sizeof(u32))
    {
        *pw = 0;
        pw++;
        size -= sizeof(u32);
    }

    // Byte epilogue: the remaining 1..3 bytes.
    p = (volatile u8 *)(uintptr_t)pw;
    while (size != 0)
    {
        *p = 0;
        p++;
        size--;
    }
}

void memerase_safe(void *dest, size_t size)
{
    // Write through a volatile-qualified pointer: accesses to volatile objects
    // are observable side effects, so the compiler is not permitted to elide
    // these stores even if 'dest' is never read again (dead-store elimination).
    volatile u8 *p = (volatile u8 *)dest;
    u32 rnd = 0;

    for (size_t i = 0; i < size; i++)
    {
        // Refill once we have consumed a full word (sizeof(u32) bytes),
        // then dispense that fresh word one byte at a time below.
        if ((i % sizeof(u32)) == 0)
        {
            if (prng_read(&rnd) != TS_TRUE)
            {   // an unseeded PRNG must not stop the erase - zeros overwrite the
                // data just as well, they are only a weaker fill pattern
                rnd = 0;
            }
        }
        p[i] = (u8)(rnd & 0xFFU);
        rnd >>= 8;
    }
}

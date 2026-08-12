/**
 * @file util.c
 * @author Tropic Square
 * @brief Some universal utility source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "util.h"
#include "os.h"

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

void memclear32(u32 *dest, u32 size)
{
    OS_SANITY_ALIGNED((uintptr_t)(dest));
    OS_SANITY_ALIGNED(size);
    
    for (u32 i=0; i<size; i+=sizeof(u32), dest++)
    {
        *dest=0;
    }
}


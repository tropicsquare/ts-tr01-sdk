/**
 * @file util.c
 * @author Tropic Square
 * @brief Some universal utility source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "util.h"

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

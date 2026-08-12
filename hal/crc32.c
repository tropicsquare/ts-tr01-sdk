/**
 * @file crc32.c
 * @author Tropic Square
 * @brief CRC32 computation file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "crc32.h"

#define CRC32_POLYNOMIAL 0xEDB88320

u32 crc32(const u8 *data, size_t len) 
{
    u32 crc = 0xFFFFFFFF;

    OS_SANITY_NULL(data);

    for (size_t i = 0; i < len; i++) 
    {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) 
        {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLYNOMIAL : 0);
        }
    }
    return (crc ^ 0xFFFFFFFF);
}


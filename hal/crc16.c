/**
 * @file crc16.c
 * @author Tropic Square
 * @brief CRC checksum computation header file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "crc16.h"

u16 crc16_byte(u8 data, u16 crc)
{ 
    u16 current_byte = data;
    int i = 8;

    crc ^= current_byte << 8;
    do
    {
        if (crc & 0x8000) {
            crc <<= 1;
            crc ^= CRC16_POLYNOMIAL;
        }
        else {
            crc <<= 1;
        }
    } while (--i);

    return crc;
}

u16 crc16(const u8 *data, size_t len)
{
    u16 crc = CRC16_INITIAL_VAL;

    OS_SANITY_NULL(data);

    while (len-- > 0)
    {
        crc = crc16_byte(*data++, crc);
    }

#if CRC16_FINAL_XOR_VALUE
    return crc ^ CRC16_FINAL_XOR_VALUE;
#else
    return crc;
#endif
}

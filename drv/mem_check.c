/**
 * @file mem_check.c
 * @author Tropic Square
 * @brief Memory check functions.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io_ops.h>
#include <mem_check.h>


void fill_mem_32(size_t base, size_t size, u32 value)
{
    // Iterates each word
    for (size_t i = 0; i < size / sizeof(u32); i++)
    {
        PTR32_T(OFFSET_IO(base, i)) = value;
    }
}


bool check_mem_32(size_t base, size_t size, u32 value)
{
    bool ret_val = true;

    // Iterates each word
    for (size_t i = 0; i < size / sizeof(u32); i++)
    {
        u32 read_value = PTR32_T(OFFSET_IO(base, i));
        if (read_value != value)
        {
            ret_val = false;
        }
    }

    return ret_val;
}


bool test_mem_8(size_t base, size_t size, uint8_t value, u32 prev_value)
{
    u32 exp_val[4] = {
        (prev_value & 0xFFFFFF00) | value,
        (prev_value & 0xFFFF0000) | value | (value << 8),
        (prev_value & 0xFF000000) | value | (value << 8) | (value << 16),
        value | (value << 8) | (value << 16) | (value << 24)
    };

    // Iterates each byte
    for (size_t i = 0; i < size; i++)
    {
        PTR8_T(base + i) = value;
        // Read back always 32-bit and check only the according byte was written!
        if (exp_val[i % 4] != PTR32_T(base + i - (i % 4)))
        {
            return false;
        }
    }

    return true;
}


bool test_mem_16(size_t base, size_t size, uint16_t value, u32 prev_value)
{
    u32 exp_val[2] = {
        (prev_value & 0xFFFF0000) | value,
        value | (value << 16)
    };

    // Iterates each byte
    for (size_t i = 0; i < size; i += 2)
    {
        PTR16_T(base + i) = value;
        // Read back always 32-bit and check only the according half-word was written!
        if (exp_val[(i % 4) / 2] != PTR32_T(base + i - (i % 4)))
        {
            return false;
        }
    }

    return true;
}

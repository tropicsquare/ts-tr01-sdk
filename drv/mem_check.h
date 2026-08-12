/**
 * @file mem_check.h
 * @author Tropic Square
 * @brief Memory check functions.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "type.h"

#ifndef MEM_CHECK_H
#define MEM_CHECK_H

/**
 * @brief Fill memory by constant fixed value. Use 32-bit access.
 *
 * @param base Memory base address
 * @param size Size of memory (in bytes)
 * @param value Value to write to each address
 */
void fill_mem_32(size_t base, size_t size, u32 value);

/**
 * @brief Check memory contains constant at all addresses. Use 32-bit access.
 *
 * @param base Memory base address
 * @param size Size of memory (in bytes)
 * @param value Value to expect in each address
 * @returns True - If all addresses contain expected value, False otherwise
 */
bool check_mem_32(size_t base, size_t size, u32 value);

/**
 * @brief Test memory by access with 8 bit values.
 *
 * @param base Memory base address
 * @param size Size of memory (in bytes)
 * @param value Value to write to each address
 * @param prev_value Previous value of each memory word!
 */
bool test_mem_8(size_t base, size_t size, uint8_t value, u32 prev_value);

/**
 * @brief Test memory by access with 16 bit values.
 *
 * @param base Memory base address
 * @param size Size of memory (in bytes)
 * @param value Value to write to each address
 * @param prev_value Previous value of each memory word!
 */
bool test_mem_16(size_t base, size_t size, uint16_t value, u32 prev_value);

#endif // ! MEM_CHECK_H

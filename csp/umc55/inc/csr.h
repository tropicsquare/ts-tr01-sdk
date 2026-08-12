/**
 * @file csr.h
 * @author Tropic Square
 * @brief Control and Status Registers access utility functions.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef CSR_H
#define CSR_H

#include "type.h"
#include "cpuss_defs.h"
#include "cpu.h"

/**
 * Read data from CPU configuration and status register (CSR).
 *
 * @param[in] csr_id: CSR to read
 * @returns data from CSR (u32)
 */
inline u32 __attribute__((always_inline)) cpu_read_csr(const csr_enum_t csr_id) 
{
    u32 data;
    asm volatile ("csrr %0, %1" : "=r"(data) : "i"(csr_id): "memory");
    return data;
}

/**
 * Write data to CPU configuration and status register (CSR).
 *
 * @param[in] csr_id: CSR to write
 * @param[in] data: Data to write (u32)
 */
inline void __attribute__((always_inline)) cpu_write_csr(const csr_enum_t csr_id, u32 data) 
{
    asm volatile ("csrw %0, %1" : : "i"(csr_id), "r"(data): "memory");
}

/**
 * Set bits in CPU configuration and status register (CSR).
 *
 * @param[in] csr_id: CSR to write
 * @param[in] mask: bits to set (u32)
 */
inline void __attribute__((always_inline)) cpu_set_bits_csr(const csr_enum_t csr_id, u32 mask) 
{
    asm volatile ("csrs %0, %1" : : "i"(csr_id), "r"(mask): "memory");
}

/**
 * Clear bits in CPU configuration and status register (CSR).
 *
 * @param[in] csr_id: CSR to write
 * @param[in] mask: bits to clear (u32)
 */
inline void __attribute__((always_inline)) cpu_clear_bits_csr(const csr_enum_t csr_id, u32 mask) 
{
    asm volatile("csrc %0, %1" : :"i"(csr_id), "r"(mask): "memory");
}

#endif // ! CSR_H


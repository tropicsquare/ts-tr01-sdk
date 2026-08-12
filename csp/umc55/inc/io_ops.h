/**
 * @file io_ops.h
 * @author Tropic Square
 * @brief Memory mapped IO operations, access to peripheral regions.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef IO_OPS_H
#define IO_OPS_H

#include "type.h"

/** 
 * @name  IO space accesses
 * @note  Compiler can't optimize them, always explicit
 * @warning TASSIC peripherals only support 32-bit access!
 * @todo Type checking that value passed to addr is somehow IO address,
        needs to be figured out how to do it. Maybe defining each IO
        address must be of some type defined by us ??
 */
///@{

/** @brief Read-modify-write accessors, can be used as pointer. */
#define PTR32_T(address) *((volatile u32*)(address))

#define PTR16_T(address) *((volatile u16*)(address))

#define PTR8_T(address) *((volatile u8*)(address))

/** @brief Raw read/write accesses. */
#define IO_WRITE_32(addr, data) {\
        *(volatile uint32_t *)(addr) = ((uint32_t)(data));\
    }

#define IO_READ_32(addr) (*((volatile uint32_t*)(addr)))

///@}

/**
 * Offset from IO location by "num_locations" locations
 * @warning This assumes 32-bit access!
 */
#define OFFSET_IO(base, num_locations) (((u32) (base)) + 0x4 * (num_locations))

/** 
 * @name  Memory space accesses
 * @note  Not volatile, can be optimized by compiler
 */
///@{
#define MEM_WRITE_32(addr, data) {\
    *((u32 *)(addr)) = ((u32)(data));\
}
#define MEM_READ_32(addr) (*((u32*)(addr)))

#define MEM_WRITE_16(addr, data) {\
    *((u16 *)(addr)) = ((u16)(data));\
}
#define MEM_READ_16(addr) (*((u16*)(addr)))

#define MEM_WRITE_8(addr, data) {\
    *((u8 *)(addr)) = ((u8)(data));\
}
#define MEM_READ_8(addr) (*((u8*)(addr)))
///@}
#endif // ! IO_OPS_H

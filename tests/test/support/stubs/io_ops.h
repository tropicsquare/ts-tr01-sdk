/**
 * @file io_ops.h
 * @brief Host stub for io_ops.h - routes MMIO accesses to a peripheral model.
 *
 * The real accessors dereference the address as a volatile pointer, which cannot
 * work on a host: the peripheral addresses are not mapped, and the u32 address
 * would truncate a 64-bit host pointer. Every IO access is routed to
 * mmio_read_32() / mmio_write_32() instead, which the test implements to model
 * the peripheral under test - including the side effects of a write.
 *
 * @warning PTR32_T() gives direct access to the register storage, so a write
 *          through it does NOT run the model side effects. It is provided only
 *          for the read-modify-write accesses of the drivers which use it.
 */

#ifndef IO_OPS_H
#define IO_OPS_H

#include "type.h"

/** @brief Read a peripheral register, implemented by the test. */
u32 mmio_read_32(u32 addr);

/** @brief Write a peripheral register, implemented by the test. */
void mmio_write_32(u32 addr, u32 data);

/** @brief Direct access to the register storage, implemented by the test. */
volatile u32 *mmio_ptr_32(u32 addr);

#define PTR32_T(address)            (*mmio_ptr_32((u32)(address)))

#define IO_WRITE_32(addr, data)     mmio_write_32((u32)(addr), (u32)(data))
#define IO_READ_32(addr)            mmio_read_32((u32)(addr))

#define OFFSET_IO(base, num_locations) (((u32) (base)) + (0x4 * (num_locations)))

/* Memory space accesses - plain pointers, usable as they are on a host. */
#define MEM_WRITE_32(addr, data) {\
    *((u32 *)(addr)) = ((u32)(data));\
}
#define MEM_READ_32(addr) (*((u32*)(addr)))

#endif /* IO_OPS_H */

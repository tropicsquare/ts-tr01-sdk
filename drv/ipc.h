/**
 * @file ipc.h
 * @author Tropic Square
 * @brief IPC (Inter Process communication Channel) driver header file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef IPC_H
#define IPC_H

#include "type.h"
#include "io_ops.h"
#include "ipc_regs.h"

#define IPC_BASE_ADDRESS 0x00200000

#define IPC_MAX_MSG_SIZE_BYTES 255
#define IPC_MAX_MSG_SIZE_WORDS (IPC_MAX_MSG_SIZE_BYTES / sizeof(u32))

/**
 * @brief Initialize the driver
 * @note  Need to be called to prevent linker remove ipc_memory allocation.
 */
void ipc_init(void);

/**
 * @brief Sends messages to parent process
 * @note  Does not block until parent process has read message.
 * @param msg Message format to be printed.
 * @param ... (additional arguments) arguments that will replace the format specifier in the msg string.
 *        only the %x, %u, %d, %c and %s format specifiers are supported.
 */
void ipc_print_msg(const char *msg, ...);

/**
 * @brief Sends one int number (4 bytes) to parent process
 *
 * @param num Number to be printed.
 */
void ipc_print_num(u32 num);

/**
 * @brief Marks test as passed for parent process
 */
void ipc_test_passed(void);

/**
 * @brief Marks test as failed for parent process
 */
void ipc_test_failed(void);

/**
 * @brief Sends an array of unsigned 32 bit integers to parent process
 * @note  Blocks until parent process has read data
 * @param data array to be sent
 * @param size number of array elements, max 63 (252 bytes)
 */
void ipc_transmit_data(u32 size, const u32 *data);

/**
 * Bulk transfer enables bigger data size sent in a single TPDI transfer
 * This funcion does not have the limit of 252 max bytes sent.
 * @brief Sends an array of unsigned 32 bit integers to parent process
 * @note  Blocks until parent process has read data
 * @param data array to be sent
 * @param size number of array elements
 */
void ipc_transmit_data_bulk(u32 size, const u32 *data);

/**
 * Bulk transfer enables bigger data size sent in a single TPDI transfer
 * This funcion does not have the limit of 252 max bytes sent.
 * @brief Sends an array of unsigned 32 bit integers to parent process
 * @note  Does not block until parent process has read message.
 * @note  The data buffer must not be changed from the FW until the transfer has completed.
 * @param data array to be sent
 * @param size number of array elements
 */
void ipc_print_data_bulk(u32 size, const u32 *data);

/**
 * @brief Sends an array of unsigned 8 bit integers to parent process
 * @note  Blocks until parent process has read data
 * @param data array to be sent
 * @param size number of array elements, max 63 bytes
 */
void ipc_transmit_data_u8(u32 size, const u8 *data);

/**
 * @brief Gets a random number from parent process
 * @note  Blocks until parent process sends random number
 * @return random_number
 */
u32 ipc_get_random_num(void) TS_CHECK_RETVAL;

/**
 * @brief Gets a u32 array from parent process
 * @note  Blocks until parent process sends entire array
 * @param size of array to receive, max 63 (252 bytes)
 * @param data array to write into
 */
void ipc_receive_data(u32 size, u32 *data);

/**
 * @brief Gets a u32 number from parent process
 * @note  Blocks until parent process sends number
 */
u32 ipc_receive_num(void) TS_CHECK_RETVAL;

#endif /* ! IPC_H */

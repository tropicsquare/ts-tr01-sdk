/**
 * @file cpb.h
 * @author Tropic Square
 * @brief Header file for CPB (Command Processing Block) operations.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef CPB_H
#define CPB_H

#include "type.h"

/** @defgroup CPB_Offsets CPB Memory Offsets */
/**@{*/
#define CPB_OFFSET_REGISTERS       0x0000 /**< Offset for registers. */
#define CPB_OFFSET_DESCRIPTOR_MEM  0x0100 /**< Offset for descriptor memory. */
#define CPB_OFFSET_COMMAND_BUFFER  0x1000 /**< Offset for command buffer. */
#define CPB_OFFSET_RESULT_BUFFER   0x1800 /**< Offset for result buffer. */
/**@}*/

/** @defgroup CPB_BufferSizes CPB Buffer Sizes */
/**@{*/
#define CPB_COMMAND_BUFFER_SIZE    144 /**< Size of the command buffer. */
#define CPB_RESULT_BUFFER_SIZE     128 /**< Size of the result buffer. */
#define CPB_NUM_DESCRIPTORS         40 /**< Number of descriptors. */
/**@}*/

/** @defgroup CPB_Descriptor CPB Descriptor Memory */
/**@{*/
#define CPB_DESCRIPTOR_MEM_DID_MASK  GENMASK(7, 0)   /**< Mask for descriptor ID. */
#define CPB_DESCRIPTOR_MEM_DID_POS   0               /**< Position for descriptor ID. */
#define CPB_DESCRIPTOR_MEM_DCOIX_MASK GENMASK(11, 8) /**< Mask for descriptor COIX. */
#define CPB_DESCRIPTOR_MEM_DCOIX_POS  8              /**< Position for descriptor COIX. */
#define CPB_DESCRIPTOR_MEM_DLSB_MASK  GENMASK(15, 12) /**< Mask for descriptor LSB. */
#define CPB_DESCRIPTOR_MEM_DLSB_POS   12              /**< Position for descriptor LSB. */
#define CPB_DESCRIPTOR_MEM_DMASK_MASK GENMASK(23, 16) /**< Mask for descriptor DMASK. */
#define CPB_DESCRIPTOR_MEM_DMASK_POS  16              /**< Position for descriptor DMASK. */
/**@}*/

/** @defgroup CPB_Results CPB Result Codes */
/**@{*/
#define CPB_RESULT_AUTH_OK          0 /**< Authentication successful. */
#define CPB_RESULT_BUSY             1 /**< CPB is busy. */
#define CPB_RESULT_AUTH_FAILED      2 /**< Authentication failed. */
#define CPB_RESULT_INVALID_COMMAND  3 /**< Invalid command received. */
#define CPB_RESULT_DESCRIPTOR_ERR   4 /**< Descriptor error occurred. */
/**@}*/

/**
 * @brief Initializes the CPB module.
 * @param error_code Error code to initialize with.
 */
void cpb_init(u8 error_code);

/**
 * @brief Suspends CPB operation.
 */
void cpb_suspend(void);

/**
 * @brief Wakes up CPB from suspended state.
 */
void cpb_wakeup(void);

/**
 * @brief Sets a descriptor in CPB.
 * @param id Descriptor ID.
 * @param value Descriptor value.
 */
void cpb_set_descriptor(u32 id, u32 value);

/**
 * @brief Clears all descriptors in CPB.
 */
void cpb_clear_descriptors(void);

/**
 * @brief Retrieves the current command from CPB.
 * @return The command code.
 */
u8 cpb_get_command(void) TS_CHECK_RETVAL;

/**
 * @brief Resets the result buffer.
 */
void cpb_result_reset(void);

/**
 * @brief Retrieves the result code from CPB.
 * @return The result code.
 */
u8 cpb_result_code(void) TS_CHECK_RETVAL;

/**
 * @brief Sets the command buffer pointer.
 * @param offset Offset value to set.
 */
void cpb_command_pointer(size_t offset);

/**
 * @brief Sets the result buffer pointer.
 * @param offset Offset value to set.
 */
void cpb_result_pointer(size_t offset);

/**
 * @brief Sets the CPB error code.
 * @param value Error code value to set.
 */
void cpb_set_error_code(u8 value);

/**
 * @brief Reads data from CPB.
 * @param dest Pointer to destination buffer.
 * @param offset Offset from where to read.
 * @param len Number of bytes to read.
 */
void cpb_read_data(u8 *dest, size_t offset, size_t len);

/**
 * @brief Writes data to CPB.
 * @param src Pointer to source buffer.
 * @param offset Offset to write to.
 * @param len Number of bytes to write.
 */
void cpb_write_data(u8 *src, size_t offset, size_t len);


#endif // ! CPB_H


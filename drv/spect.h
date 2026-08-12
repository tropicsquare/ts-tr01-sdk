/**
 * @file spect.h
 * @author Tropic Square
 * @brief Spect header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SPECT_H
#define SPECT_H

#include "type.h"
#include "spect_ops_constants.h"

#define SPECT_RAM_IN_SIZE   (0x800)
#define SPECT_RAM_OUT_SIZE  (0x200)
#define SPECT_I_MEM_SIZE    (0x3000)

/**
 * @name Offsets to DATA RAM IN
 * 
 */
///@{
#define SPECT_OFFSET_DATA_IN            0x00
#define SPECT_OFFSET_CFG_WORD           0x100
///@}

#define SPECT_CFG_WORD_OP_ID_MASK                GENMASK(7,0)
#define SPECT_CFG_WORD_OP_ID_POS                 0
#define SPECT_CFG_WORD_OUT_DST_MASK              GENMASK(11,8)
#define SPECT_CFG_WORD_OUT_DST_POS               8
#define SPECT_CFG_WORD_IN_SRC_MASK               GENMASK(15,12)
#define SPECT_CFG_WORD_IN_SRC_POS                12
#define SPECT_CFG_WORD_IN_SIZE_MASK              GENMASK(31,16)
#define SPECT_CFG_WORD_IN_SIZE_POS               16

/**
 * @name Offsets to DATA RAM OUT
 * 
 */
///@{
#define SPECT_OFFSET_OP_DATA_OUT        0x00
#define SPECT_OFFSET_RES_WORD           0x100
///@}

#define SPECT_RES_WORD_OP_STATUS_MASK     GENMASK(7,0)
#define SPECT_RES_WORD_OP_STATUS_POS      0
#define SPECT_RES_WORD_DATA_OUT_SIZE_MASK GENMASK(31,16)
#define SPECT_RES_WORD_DATA_OUT_SIZE_POS  16

#define SPECT_OFFSET_OP_STATUS          0x100

#define SPECT_INPUT_SRC_DATA_IN         0
#define SPECT_INPUT_SRC_CMD_BUFFER      4
#define SPECT_OUTPUT_DST_DATA_OUT       1
#define SPECT_OUTPUT_DST_CMD_BUFFER     5

#define SPECT_DATA_CHUNK_SIZE (128)
#define SPECT_SHA512_CHUNK_SIZE (128)
#define SPECT_KEY_SIZE (32)
#define SPECT_HASH_SIZE (64)

#define SPECT_SHA512_PADDING_CHARACTER 0x80

/**
 * @name SPECT result codes
 * @note For SPECT_OP_ID_* and other defines use generator from ts-spect-fw.git.
 */
typedef enum {
    SPECT_OP_OK                         = 0x00,
    SPECT_OP_CONTEXT_ERROR              = 0xF1,
    SPECT_OP_KBUS_ERROR                 = 0xF2,
    SPECT_OP_INVALID_OP_ID              = 0xF3,
    SPECT_OP_INVALID_CURVE              = 0xF4,
    SPECT_OP_GRV_ERROR                  = 0xF5,
    SPECT_OP_X25519_INVALID_PRIV_KEY    = 0x11,
    SPECT_OP_X25519_INVALID_PUB_KEY     = 0x12,
    SPECT_OP_ECDSA_INVALID_ECDSA_NONCE  = 0x21,
    SPECT_OP_ECDSA_INVALID_ECDSA_R      = 0x22,
    SPECT_OP_ECDSA_INVALID_ECDSA_S      = 0x23,
    SPECT_OP_ECDSA_FINAL_VERIFY_FAIL    = 0x24,
    SPECT_OP_EDDSA_INVALID_PRIV_KEY_S   = 0x34,
    SPECT_OP_EDDSA_INVALID_PUB_KEY_A    = 0x35,
    SPECT_OP_EDDSA_FINAL_VERIFY_FAIL    = 0x36,
    SPECT_OP_POINT_INTEGRITY_ERROR      = 0x41
} spect_result_code_t;

typedef u8 spect_op_id_t;

/**
 * @brief Initialize SPECT coprocessor.
 *
 * Performs:
 *  - Enables clock for SPECT
 *  - Configures IRQ and registers
 *  - Clears its RAM memory
 *
 * @returns TS_FALSE if some misbehaviour detected, TS_TRUE otherwise
 */
ts_bool spect_init(void);

/**
 * @brief Enables clock for SPECT.
 */
void spect_wakeup(void);

/**
 * @brief Disables clock for SPECT.
 */
void spect_suspend(void);

/**
 * @brief Issue SOFT-RESET command.
 */
void spect_reset(void);


/**
 * @brief Execute already configured command.
 */
void spect_exec_cmd(void);

/**
 * @brief Initialize requested SPECT operation.
 * @param[in] op_id SPECT_OP_ID_* value
 * @param[in] size Data size for the operation
* 
 */
void spect_op_init(spect_op_id_t op_id, size_t size);


/**
 * @brief Initialize requested SPECT operation.
 * In this case the input/output will be CPB command/result buffer
 * @param[in] op_id SPECT_OP_ID_* value
 * @param[in] size Data size for the operation
 */
void spect_cpb_op_init(spect_op_id_t op_id, size_t size);

/**
 * @brief Prosess operation and wait for done.
 * @param[in] op_id SPECT_OP_ID_* value
 * @param[in] size Data size for the operation
 */
ts_bool spect_cpb_op_task(spect_op_id_t op_id, size_t size);

/**
 * @brief Read one u32 word from DRAM OUT memory.
 * @param[in] offset Number of bytes offset to begin.
 */
u32  spect_read_dram_out_u32(u32 offset);


/**
 * @brief Read data from DRAM OUT memory.
 * @param[out] dest The destination to copy data.
 * @param[in] offset Number of bytes offset to begin.
 * @param[in] len Number of bytes to copy.
 */
void spect_read_dram_out(u8 *dest, u32 offset, size_t len);

/**
 * @brief Write one u32 word to DRAM IN memory.
 * @param[in] offset Number of bytes offset to begin.
 * @param[in] value U32 value to write.
 */
void spect_write_dram_in_u32(u32 offset, u32 value);

/**
 * @brief Write data to DRAM IN memory.
 * @param[in] offset Number of bytes offset to begin.
 * @param[in] data The source of data to copy.
 * @param[in] len Number of bytes to copy.
 */
void spect_write_dram_in(u32 offset, const u8 *data, size_t len);

/**
 * @brief Write SPECT firmware to its destination RAM memory.
 * @param[in] offset Number of bytes offset to begin.
 * @param[in] data The source of data to copy.
 * @param[in] len Number of bytes to copy.
 */
void spect_write_fw(u32 offset, const u32 *data, size_t len);

/**
 * @brief Wait for SPECT operation done.
 * Uses default maximal timeout.
 */
ts_bool spect_wait_done(void);

/**
 * @brief Read the result code when operation finished.
 */
spect_result_code_t spect_result_code(void);

/**
 * @brief Read the size of result when operation finished.
 */
size_t spect_result_size(void);

/**
 * @brief Return physical address where to store SPECT firmware.
 */
u32 *spect_address(void);

/*******************************************************
 * API for TASSIC Top verification. Do not remove!
*******************************************************/
/**
 * @name API for TASSIC Top verification
 * @warning Do not remove!
 */
///@{
ts_bool spect_get_done_flag(void);
ts_bool spect_get_error_flag(void);
///@}

#endif // ! SPECT_H

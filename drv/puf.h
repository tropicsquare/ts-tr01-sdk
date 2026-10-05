/**
 * @file puf.h
 * @author Tropic Square
 * @brief PUF HW driver
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef PUF_H
#define PUF_H

#include "type.h"

#define PUF_DATA_SIZE32  (8) // u32 words
#define PUF_MASK_SIZE32 (32) // u32 words

/**
    @brief Initialize and wake up PUF

    @param[in] read_rate Configures the delay in cycles between reading PUF bits.
 */
void puf_init(u32 read_rate);

/**
 * @brief Wakes up PUF from suspension.
 */
void puf_wakeup(void);

/**
 * @brief Suspends PUF operation.
 */
void puf_suspend(void);

/**
 * @brief Read PUF data result for provided challenge.
 *
 * @param[in] challenge The challenge word.
 * @param[out] out Destination for data reading.
 */
ts_bool puf_read_value(u32 out[PUF_DATA_SIZE32], u32 challenge) TS_CHECK_RETVAL;

/**
 * @brief Set the reliable bit mask for PUF HW.
 *
 * @param[in] mask 32 words of appropriate PUF mask.
 * @note Mask may be set only once for multiple challenges.
 */
void puf_set_mask(u32 mask[PUF_MASK_SIZE32]);

/**
 * @brief Set syndrome for error correction.
 *
 * @param[in] syndrome Syndrome to set.
 * @note Syndrome may be set only once for multiple challenges.
 */
void puf_set_syndrome(u32 syndrome);

#endif // ! PUF_H


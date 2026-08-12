/**
 * @file otp.h
 * @brief One-Time Programmable (OTP) controller hardware driver interface.
 *
 * This header provides function prototypes and constants for accessing
 * and managing the OTP memory of the TROPIC01 chip. The OTP controller
 * allows reading, writing, and verifying programmed data.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 *
 * @author: Tropic Square
 */

#ifndef OTP_H
#define OTP_H

#include "type.h"
#include "hw.h"

#define OTP_SIZE (0x2000)


/**
 * @name Default values for current frequency MHz
 * 
 */
///@{
/** @brief Chip frequency in MHz. */
#define OTP_CLOCK_MHZ       (HW_CLOCK_MHZ)
#define OTP_CTRL_TIME_10US  (OTP_CLOCK_MHZ * 10)
#define OTP_CTRL_TIME_1US   (OTP_CLOCK_MHZ)
///@}

#define OTP_EMPTY_VALUE (0xFFFFFFFF)

/**
 * @brief Initialize the OTP controller.
 *
 * Sets up the OTP hardware interface and prepares it for read/write operations.
 * Must be called before any other OTP-related functions.
 * 
 * @note: Does not initialize OTP write timing, see otp_timing_init()
 *
 */
void otp_init(void);

/**
 * @brief Initialize OTP programming pulse duration.
 *
 * Must be called before any OTP write attempt.
 */
void otp_timing_init(void);

/**
 * @brief Suspend the OTP controller.
 *
 * Puts the OTP hardware into a low-power or idle state.
 * Should be called before entering a low-power mode.
 */
void otp_suspend(void);

/**
 * @brief Wake up the OTP controller.
 *
 * Restores the OTP hardware from suspend mode to normal operation.
 */
void otp_wakeup(void);

/**
 * @brief Set data scrambling
 *
 * The words order will be reordered.
 *
 * @param[in] seed is the per-chip constant value of pseudo-random bytes (PUF based).
 * @note Number of seed bytes depends on number of ADDR_* fields in OTP_CTRL_SCRAM_* registers.
 */
void otp_init_scrambling(u8 *seed);


/**
 * @brief Read I-Config type word.
 *
 * The word contains 8 bits.
 *
 * @param[in] addr Address.
 */
u8 otp_read_bit_field(u32 addr);

/**
 * @brief Write a 32-bit word into OTP memory.
 *
 * Programs a single word at the specified OTP address.
 *
 * @param[in] addr OTP address to write to.
 * @param[in] data 32-bit data word to be written.
 */
void otp_write_word(u32 addr, u32 data);


/**
 * @brief Write and verify a 32-bit word in OTP memory.
 *
 * Writes a word to the specified address and reads it back for verification.
 *
 * @param[in] addr OTP address to write to.
 * @param[in] data 32-bit data word to be written.
 * @return `TS_TRUE` if the verification succeeds, `TS_FALSE` otherwise.
 */
ts_bool otp_write_word_verify(u32 addr, u32 data);

/**
 * @brief Read single word from OTP.
 *
 * @param[in] addr Address.
 */
u32 otp_read_word(u32 addr);

/**
 * @brief Read block of data from OTP.
 *
 * @param[out] dest Destination for data.
 * @param[in] addr Address.
 * @param[in] size Number of bytes to read (multiple of 4).
 */
void otp_read_data(u8 *dest, u32 addr, size_t size);

#endif // ! OTP_H

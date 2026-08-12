/**
 * @file msg.h
 * @author Tropic Square
 * @brief Message handling interface (SPI communication) header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef MSG_H
#define MSG_H

#include "type.h"
#include "ts_l2_defs.h"


/**
 * @struct msg_t
 * @brief Represents a protocol message for input/output communication.
 *
 * This structure holds either a command (for input messages) or a result code
 * (for output messages), along with message length and associated data payload.
 */
typedef struct {
    /** @brief  Command (for input) or result code (for output). */
    u8 hdr;
    u8 len;
    u8 data[TS_L2_MAX_LEN_DATA];
} msg_t;

/**
 * @brief Initialize the message module.
 *
 * Should be called once during system setup to prepare message buffers and state.
 */
void msg_init(void);

/**
 * @brief Clear a message structure.
 *
 * Resets the contents of the given message to default/zero values.
 *
 * @param msg Pointer to the message to clear.
 */
void msg_clear(msg_t *msg);

/**
 * @brief Check if RX stream is idle (no data is currently being received).
 *
 * @returns TS_TRUE if RX is idle, TS_FALSE otherwise.
 */
ts_bool msg_rx_idle(void);

/**
 * @brief Check if RX has completed message reception.
 *
 * @returns TS_TRUE if message reception is done, TS_FALSE otherwise.
 */
ts_bool msg_rx_done(void);

/**
 * @brief Check if the last received message was received correctly.
 *
 * @returns TS_TRUE if message passed all validation checks.
 */
ts_bool msg_rx_ok(void);

/**
 * @brief Check if the last received message has a CRC error.
 *
 * @returns TS_TRUE if CRC error detected, TS_FALSE otherwise.
 */
ts_bool msg_rx_crc_error(void);

/**
 * @brief Check if the last received message has a length error.
 *
 * @returns TS_TRUE if message length is invalid, TS_FALSE otherwise.
 */
ts_bool msg_rx_len_error(void);

/**
 * @brief Get RX buffer for received messages.
 *
 * We have single static buffer for receiving messages.
 *
 * @returns Pointer to buffer for RX messages.
 */
msg_t *msg_get_rx_buffer(void);

/**
 * @brief Reset the RX logic and clear any in-progress reception.
 */
void msg_rx_reset(void);

/**
 * @brief Check if the TX stream is idle (nothing to send).
 *
 * Indicates that all data has been transmitted or is in the hardware TX buffer.
 *
 * @returns TS_TRUE if TX is idle, TS_FALSE otherwise.
 */
ts_bool msg_tx_is_idle(void);

#define msg_tx_idle msg_tx_is_idle // backward compatibility support

/**
 * @brief Send a message by placing it into the TX queue.
 *
 * The message will be made available for the master to pull.
 *
 * @param msg Pointer to the message to send.
 * @returns TS_TRUE if the message was queued successfully.
 */
ts_bool msg_tx_send(msg_t *msg);

#endif // ! MSG_H

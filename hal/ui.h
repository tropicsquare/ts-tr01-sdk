/**
 * @file ui.h
 * @author Tropic Square
 * @brief User Interface HAL module (SPI communication).
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef UI_H
#define UI_H

#include "common.h"
#include "msg.h"

typedef ts_bool (*ui_callback_t) (msg_t *msg);

/**
 * @brief Module main init.
 *
 * To be called one during start.
 *
 * @param[in] callback Function to be called when received L2 packet.
 */
void ui_init(ui_callback_t callback);

/**
 * @brief Send response.
 *
 * @param[in] msg L2 response to be send.
 */
void ui_response(msg_t *msg);

/**
 * @brief Send asynchronous response.
 *
 * This is useful when sending L3 packet as multiple L2 responses.
 *
 * @param[in] msg L2 response to be send.
 */
void ui_response_async(msg_t *msg);

/**
 * @brief Check if next L3 chunk may be send.
 *
 * @returns TS_TRUE when resend needed and cant send new chunk
 */
ts_bool ui_response_async_blocked(void);

/**
 * @brief Request to resend last L2 response.
 *
 * @returns TS_TRUE when resend was successful.
 */
ts_bool ui_resend(void);

/**
 * @brief Check if receiving is idle.
 *
 * @returns TS_TRUE when no receiving in progress.
 */
ts_bool ui_idle(void);

/**
 * @brief Check if all communication is done.
 * @returns TS_TRUE when no receiving or transmitting in progress.
 */
ts_bool ui_done(void);

/**
 * @brief Main module task which has to be called regularly.
 */
void ui_task(void);

#endif // ! UI_H

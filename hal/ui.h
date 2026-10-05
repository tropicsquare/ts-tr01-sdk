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

/**
 * @brief Handler of a received L2 packet.
 *
 * @note The handler answers the host itself (via ui_response() or ui_resend()),
 *       so there is no result left for ui_task() to act on.
 */
typedef void (*ui_callback_t) (msg_t *msg);

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
ts_bool ui_response_async_blocked(void) TS_CHECK_RETVAL;

/**
 * @brief Request to resend last L2 response.
 *
 * @returns TS_TRUE when resend was successful.
 */
ts_bool ui_resend(void) TS_CHECK_RETVAL;

/**
 * @brief Read-and-clear the "a RESEND_REQ was just served" flag.
 *
 * Set by ui_resend() whenever it actually re-sends the stored response. The L3
 * streaming task uses it to defer the next-chunk advance by one pass, so the
 * pull of a re-sent chunk never triggers an immediate advance.
 *
 * @returns TS_TRUE if a resend was served since the last call.
 */
ts_bool ui_resend_served(void) TS_CHECK_RETVAL;

/**
 * @brief Tell the UI layer whether an L3 result stream is mid-flight.
 *
 * While active, a RESEND_REQ arms the async-retry block even if the last
 * committed chunk has already been promoted, so the next-chunk commit is
 * reliably deferred.
 *
 * @param[in] active TS_TRUE while streaming an L3 result, TS_FALSE otherwise.
 */
void ui_set_stream_active(ts_bool active);

/**
 * @brief Check if receiving is idle.
 *
 * @returns TS_TRUE when no receiving in progress.
 */
ts_bool ui_idle(void) TS_CHECK_RETVAL;

/**
 * @brief Check if all communication is done.
 * @returns TS_TRUE when no receiving or transmitting in progress.
 */
ts_bool ui_done(void) TS_CHECK_RETVAL;

/**
 * @brief Main module task which has to be called regularly.
 */
void ui_task(void);

#endif // ! UI_H

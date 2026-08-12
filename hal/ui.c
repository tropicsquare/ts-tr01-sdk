/**
 * @file ui.c
 * @author Tropic Square
 * @brief User Interface HAL module (SPI communication).
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "ui.h"
#include "msg.h"
#include "spi.h"
#include "log.h"

LOG_DEF("UI");

/** @brief Last L2 response buffer for RESEND_REQ support. */
static msg_t _msg_retry;
/** @brief Last L2 chunk of L3 response buffer for RESEND_REQ support. */
static msg_t _msg_retry_async;
/** @brief Request for retry _msg_retry_async because it was not delivered. */
static ts_bool _async_response_retry_needed; 

static ui_callback_t _rx_callback = NULL;

static msg_t *_rx_buffer = NULL; // we have single static receiving buffer, so keep it static here

void ui_init(ui_callback_t callback)
{
    OS_SANITY_NULL(callback);

    _async_response_retry_needed = TS_FALSE;
    _rx_callback = callback;
    _msg_retry.hdr = TS_L2_CMD_NONE;
    _rx_buffer = msg_get_rx_buffer();
    OS_SANITY_NULL(_rx_buffer);
    msg_init();
}

void ui_response(msg_t *msg)
{
    OS_SANITY_NULL(msg);

    _msg_retry_async.hdr = TS_L2_CMD_NONE;
    memcpy(&_msg_retry, msg, sizeof(_msg_retry));
    msg_tx_send(msg);
}

static inline ts_bool _msg_valid(msg_t *msg)
{
    return ((msg->hdr == TS_L2_CMD_NONE) ? TS_FALSE : TS_TRUE);
}

static void _update_retry_msg(void)
{
    if (_msg_valid(&_msg_retry_async) == TS_TRUE)
    {
        memcpy(&_msg_retry, &_msg_retry_async, sizeof(_msg_retry));
    }
}

void ui_response_async(msg_t *msg)
{
    OS_SANITY_NULL(msg);

    // in case there was L3 async response before, allow it for RESEND_REQ
    _update_retry_msg();
    // backup this part of L3 response to be able to resend later
    memcpy(&_msg_retry_async, msg, sizeof(_msg_retry_async));

    msg_tx_send(msg);
}

ts_bool ui_response_async_blocked(void)
{
    return (_async_response_retry_needed);
}

ts_bool ui_resend(void)
{
    if (_msg_valid(&_msg_retry) != TS_TRUE)
    {
        return TS_FALSE;
    }
    if (_msg_valid(&_msg_retry_async) == TS_TRUE)
    {   // we are in the middle of L3 response sending and RESEND_REQ happen
        // so we will need to retry last chunk later
        _async_response_retry_needed = TS_TRUE;
    }
    return (msg_tx_send(&_msg_retry));
}

ts_bool ui_idle(void)
{
    if (msg_rx_idle() != TS_TRUE)
    {
        return TS_FALSE;
    }
    return TS_TRUE;
}

void ui_task(void)
{
    if (msg_rx_done() == TS_TRUE)
    {
        if (msg_rx_ok() == TS_TRUE)
        {
            _rx_callback(_rx_buffer);
        }
        else
        {   // msg RX failed
            u8 result_code = TS_L2_RESP_NONE;

            if (msg_rx_crc_error() == TS_TRUE)
            {
                result_code = TS_L2_RESP_CRC_ERR;
            }
            if (msg_rx_len_error() == TS_TRUE)
            {   // there is more than 252B length field, receiving was skipped
                // we throw CRC_ERR result even in case CRC not checked (the packet is invalid)
                result_code = TS_L2_RESP_CRC_ERR;
            }
            if (result_code != TS_L2_RESP_NONE)
            {   // 
                msg_t msg;

                msg.hdr = result_code;
                msg.len = 0;

                ui_response(&msg);
            }
        }
        msg_rx_reset();
    }

#if DISABLE_L3 != 1    
    // in case we damaged L3 response by RESEND_REQ, we need to retry
    if ((msg_tx_is_idle() == TS_TRUE) && (msg_rx_idle() == TS_TRUE))
    {
        if (_async_response_retry_needed == TS_TRUE)
        {
            if (msg_tx_send(&_msg_retry_async) == TS_TRUE)
            {
                _async_response_retry_needed = TS_FALSE;
            }
        }
        else
        {
            _update_retry_msg();
        }
    }
#endif // DISABLE_L3 != 1 
}

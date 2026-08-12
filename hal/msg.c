/**
 * @file msg.c
 * @author Tropic Square
 * @brief Message handling interface (SPI communication) source file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "msg.h"
#include "spi.h"
#include "crc16.h"
#include "log.h"

#include <string.h>

LOG_DEF("MSG");

#define _MSG_ERR_RX_LEN 1
#define _MSG_ERR_CRC 2

/** @brief Buffer for incoming message. */
static msg_t _msg_rx;

/** @brief Size in u32 words. */
#define _BUFFER_SIZE ((TS_L2_MAX_LEN_PACKET+3)/sizeof(u32))
typedef struct {
    /** @brief Optimized to be able read by u32. */
    u32 data[_BUFFER_SIZE];
    u32 ptr;
    u32 len;
} msg_tx_stream_t;

/** @brief Buffer for response message raw data. */
static msg_tx_stream_t _tx_stream;

typedef enum {
    RX_IDLE = 0,
    RX_LEN,
    RX_DATA,
    RX_CRC1,
    RX_CRC2,
    RX_DONE,
    RX_ERROR_CRC,
    RX_ERROR_LEN
} rx_status_e;

static int _rx_status = RX_IDLE;

static void _msg_rx_byte(u8 rx_byte)
{   // rx callback called from SPI IRQ
    static u16 crc_calc = 0; // calculated CRC
    static u16 crc_rx = 0;   // received CRC
    static size_t rx_cnt = 0;   // number of data bytes already received

    switch (_rx_status)
    {
    case RX_IDLE:
        crc_calc = crc16_byte(rx_byte, CRC16_INITIAL_VAL);
        _msg_rx.hdr = rx_byte;
        // new message receiving starts, clear response queue
        spi_clear_response_queue();
        break;

    case RX_LEN:
        rx_cnt = 0;
        if (rx_byte  > TS_L2_MAX_LEN_DATA)
        {
            _rx_status = RX_ERROR_LEN;
            LOG_ERROR_NUM(_MSG_ERR_RX_LEN);
            return;
        }

        crc_calc = crc16_byte(rx_byte, crc_calc);
        _msg_rx.len = rx_byte;
        if (_msg_rx.len == 0)
        {   // no data, skip phase
            _rx_status = RX_CRC1;
            return;
        }
        break;

    case RX_DATA:
        OS_ASSERT(rx_cnt < sizeof(_msg_rx.data));

        _msg_rx.data[rx_cnt] = rx_byte;
        crc_calc = crc16_byte(rx_byte, crc_calc);

        if (++rx_cnt < _msg_rx.len)
        {
            return;
        }
        break;

    case RX_CRC1: // little endian
        crc_rx = rx_byte;
        break;

    case RX_CRC2: // second byte of CRC, it is complete now
        crc_rx |= (rx_byte << 8);
    #if CRC16_FINAL_XOR_VALUE
        crc_calc ^= CRC16_FINAL_XOR_VALUE;
    #endif // CRC16_FINAL_XOR_VALUE == 1
        if (crc_calc != crc_rx)
        {
            _rx_status = RX_ERROR_CRC;
            LOG_ERROR_NUM(_MSG_ERR_CRC);
            LOG_DEBUG("CRC rx: %04x, calc: %04x", crc_rx, crc_calc);
            return;
        }
        break;

    default:
        return;
    }

    _rx_status++;
}

static ts_bool _msg_tx_fetch_word(u32 *dest)
{   // tx callback called from SPI IRQ
    // fetch one 32 bit word
    msg_tx_stream_t *stream = &_tx_stream;

    if (stream->ptr < stream->len)
    {   // ptr and len are in u32 units
        *dest = stream->data[stream->ptr];
        stream->ptr++;
        return TS_TRUE;
    }
    return TS_FALSE;
}

void msg_init(void)
{
    _tx_stream.ptr = 0;
    _tx_stream.len = 0;
    _rx_status = RX_IDLE;

    spi_init(_msg_rx_byte, _msg_tx_fetch_word);
}

void msg_clear(msg_t *msg)
{
    OS_SANITY_NULL(msg);
    memset(msg, 0, sizeof(msg_t));
}

ts_bool msg_rx_idle(void)
{
    return ((_rx_status == RX_IDLE) ? TS_TRUE : TS_FALSE);
}

ts_bool msg_rx_done(void)
{
    if (_rx_status == RX_DONE)
    {   // receiving done, status OK
        // NOTE: message received, but CS may be still DOWN
        //       it is possible to use spi_idle() to check CS UP
        return TS_TRUE;
    }

    if (_rx_status == RX_IDLE)
    {   // receiving idle, no any activity yet
        return TS_FALSE;
    }

    if (spi_idle() == TS_TRUE)
    {   // SPI transaction done (CS is UP)
        // check again receiving done, because it may happen just now
        // The delay between CS 0 -> 1 and IRQ REQQNETYS may be up to 4 clocks
        asm volatile("nop"); // add little delay to be sure _rx_status was updated
        if (_rx_status >= RX_DONE)
        {   // receiving done, OK or error
            return TS_TRUE;
        }
        // receiving in progress, but SPI idle
        // this is error state
        _rx_status = RX_ERROR_CRC; // not enough bytes received
        return TS_TRUE;
    }
    return TS_FALSE;
}

ts_bool msg_rx_ok(void)
{
    if (spi_response_queue_empty() != TS_TRUE)
    {   // there is some new response which appear during receiving, delete it
        // NOTE: it may be L3 asynchronous response
        spi_clear_response_queue();
    }
    return ((_rx_status == RX_DONE) ? TS_TRUE : TS_FALSE);
}

ts_bool msg_rx_crc_error(void)
{
    return ((_rx_status == RX_ERROR_CRC) ? TS_TRUE : TS_FALSE);
}

ts_bool msg_rx_len_error(void)
{
    return ((_rx_status == RX_ERROR_LEN) ? TS_TRUE : TS_FALSE);
}

msg_t *msg_get_rx_buffer(void)
{
    return (&_msg_rx);
}

ts_bool msg_tx_send(msg_t *msg)
{
    u16 crc_calc = 0;
    u8 *pdata;
    u32 len;

    OS_SANITY_NULL(msg);

    if (msg->len > TS_L2_MAX_LEN_DATA)
    {
        return TS_FALSE;
    }

    // prepare data to u32 optimized buffer
    _tx_stream.ptr = 0;
    pdata =  (u8 *)_tx_stream.data;
    pdata[TS_L2_IDX_HDR] = msg->hdr;
    pdata[TS_L2_IDX_LEN] = msg->len;
    memcpy(pdata + TS_L2_IDX_DATA, msg->data, msg->len);

    len = TS_L2_IDX_DATA+msg->len;
     
    crc_calc = crc16(pdata, len);

    // add CRC as little endian
    pdata[len++] = crc_calc & 0xFF;
    pdata[len++] = (crc_calc>>8) & 0xFF;

    // padding to u32 (fill with TS_L2_RESP_NO_RESP)
    while (len & 0x3)
    {
        pdata[len++] = TS_L2_RESP_NO_RESP;
    }
    _tx_stream.len = len>>2; // convert length from bytes to words
    spi_tx_enable();

    return TS_TRUE;
}

ts_bool msg_tx_is_idle(void)
{
    if (_tx_stream.ptr != _tx_stream.len)
    {
        return TS_FALSE;
    }

    return spi_response_queue_empty();
}

void msg_rx_reset(void)
{
    _rx_status = RX_IDLE;
}


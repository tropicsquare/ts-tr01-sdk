/**
 * @file debug.c
 * @author Tropic Square
 * @brief Debug messages output source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "debug.h"

#if DEBUG_FTC 
  #include "ftc.h"
#endif // DEBUG_FTC

#if (TS_LOG_SPI == 1)

#ifndef DEBUG_BUFFER_SIZE
  /**
   * @note Default buffer size may be changed by compiler directive.
   * 
   */
  #define DEBUG_BUFFER_SIZE 256
#endif // not defined DEBUG_BUFFER_SIZE

static ascii _stream_buf[DEBUG_BUFFER_SIZE];
static u32   _stream_buf_wr = 0;
static u32   _stream_buf_rd = 0;

static inline TS_CHECK_RETVAL u32 _pinc(u32 ptr)
{   // buffer pointer loop increment
    ptr++;
    if (ptr >= DEBUG_BUFFER_SIZE)
    {
        ptr=0;
    }
    return (ptr);
}

size_t debug_fetch(u8 *data, size_t limit)
{
    if (_stream_buf_wr != _stream_buf_rd)
    {
        size_t n = 0;
        u32 i = _stream_buf_rd;

        while (n < limit)
        {
            n++;
            *data = (u8)_stream_buf[i];
            i = _pinc(i);
            data++;

            if (i == _stream_buf_wr)
                break;
        }
        _stream_buf_rd = i;
        return (n);
    }
    return (0);
}

void debug_crop(size_t limit)
{
    size_t rd = _stream_buf_rd;
    size_t wr = _stream_buf_wr;
    
    if (limit >= DEBUG_BUFFER_SIZE)
        return; // no need to crop

    // Compute current occupancy modulo buffer size
    size_t n = (wr + DEBUG_BUFFER_SIZE - rd) % DEBUG_BUFFER_SIZE;

    // If we're already within the allowed limit, nothing to do
    if (n <= limit)
        return;

    // Otherwise advance read pointer so occupancy == limit
    _stream_buf_rd = (wr + DEBUG_BUFFER_SIZE - limit) % DEBUG_BUFFER_SIZE;
}

#endif // (TS_LOG_SPI == 1)

void  debug_put_char(ascii ch)
{
    (void)ch;
#if (TS_LOG_SPI == 1)
    u32 i = _pinc(_stream_buf_wr);

    if (i == _stream_buf_rd)
    {   // buffer full
        // allow overwrite 
        _stream_buf_rd = _pinc(_stream_buf_rd);
        _stream_buf[_stream_buf_rd] = '*'; // overflow indication
    }
    _stream_buf[_stream_buf_wr] = (u8)ch;
    _stream_buf_wr = i;

#endif // (TS_LOG_SPI == 1)

#if DEBUG_FTC 
    FTC_PUT_CHAR(ch);
#endif // DEBUG_FTC
}

void debug_flush(void)
{
#if DEBUG_FTC 
    FTC_PRINT();
#endif // DEBUG_FTC
}



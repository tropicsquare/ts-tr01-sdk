/**
 * @file log.c
 * @author Tropic Square
 * @brief Logging support source file
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "os.h"
#include "log.h"

#if LOG_LEVEL > LOG_LEVEL_NONE

#include "xprintf.h"

#include <stdarg.h>

#define NL "\n"

static void _log_timestamp(void)
{
    os_timer_t t = OS_TIMER()/OS_TIMER_MS;
    OS_PRINTF("# %" PRIu32 " ", t);
}

void log_msg(const ascii *type, const ascii *id, const ascii *fmt, ... )
{
    va_list args;

    OS_SANITY_NULL(type);
    OS_SANITY_NULL(id);
    OS_SANITY_NULL(fmt);

    va_start(args, fmt);

    _log_timestamp();
    xprintf("%s(%s): ", type, id);
    xvprintf(fmt, args);
    va_end(args);
    xprintf(NL);

    OS_FLUSH();
}

void log_err_num(const ascii *id, u32 num)
{
    log_msg("E", id, "%d", num);
}

void log_dump(const ascii *id, const ascii *text, const u8 *data, size_t data_len)
{
    int n = 0;

    OS_SANITY_NULL(id);
    OS_SANITY_NULL(text);
    OS_SANITY_NULL(data);

    _log_timestamp();
    xprintf("X(%s) %s:", id, text);
    
    if (data_len > 512)
    {
        xprintf("(%d)", data_len);
        data_len = 512;
    }

    if (data_len > 32)
         OS_PRINT_MSG(NL);

    while (data_len--)
    {
        xprintf(" %x%x", *data >> 4, *data & 0xf);
        data++;
        
        n++;
        if (n == 16)
        {
            OS_PRINT_MSG(" ");
        }
        else if (n == 32)
        {
            OS_PRINT_MSG(NL);
            n = 0;
        }
    }
    xprintf(NL);
    OS_FLUSH();
}

#else //  LOG_LEVEL > LOG_LEVEL_NONE

void log_err_num(const ascii *id, u32 num)
{
    (void)id;
    (void)num;
}

#endif // LOG_LEVEL == LOG_LEVEL_NONE


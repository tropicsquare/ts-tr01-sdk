/**
 * @file xprintf.c
 * @author Tropic Square
 * @brief Simple printf implementation.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "os.h"
#include "xprintf.h"

// we dont have enough program memory for i.e. nanoprintf, so use minimal and very limited implementation
#define _VSNPRINTF ll_vsnprintf

static void _add(char **dest, const char *limit, char ch)
{
    if (*dest < limit)
    {
        **dest = ch;
        (*dest)++;
    }
}

static int ll_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist)
{   // very limited printf implementation
    // we have very limited sources
    char *limit;

    OS_SANITY_NULL(buffer);
    OS_SANITY_NULL(format);

    if (! bufsz)
    {
        return 0;
    }
    bufsz--; // keep space for ending character '\0'
    limit = buffer + bufsz;
    while (*format)
    {
        if (*format == '%')
        {
            format++;

            while ((*format >= '0') && (*format <= '9'))
                format++; // skip unsupported width specifiers

            if (*format == 'l')
                format++;

            switch (*format)
            {
                case 'c':
                    _add(&buffer, limit, va_arg(vlist, int));
                    break;

                case 'd': // "%d" does not support negative values
                case 'u':
                    {
                        u32 val = va_arg(vlist, int);
                        char tmp[10];
                        int l=0;

                        do
                        {
                            tmp[l++] = '0' + (val % 10);
                            val /= 10;
                        } while (val);

                        while (l--)
                        {
                            _add(&buffer, limit, tmp[l]);
                        }
                    }
                    break;

                case 's':
                    {
                        char *s = va_arg(vlist, char *);
                        while (*s)
                        {
                            _add(&buffer, limit, *s);
                            s++;
                        }
                    }
                    break;

                case 'x':
                    {
                        u32 val = va_arg(vlist, int);
                        u32 mask = 0xF0000000;
                        int i;
                        
                        for (i=0; i<7; i++)
                        {
                            if (val & mask)
                                break;
                            val <<= 4;
                        }
                        for (; i<8; i++)
                        {
                            int n = (val >> 28) & 0xF;
                            if (n > 9)
                                _add(&buffer, limit, (char)('a' + n - 10));

                            else
                                _add(&buffer, limit, (char)('0' + n));
                            val <<= 4;
                        }
                    }
                    break;

                default:
                    _add(&buffer, limit, *format);
                    break;
            }
        }
        else
        {
            _add(&buffer, limit, *format);
        }
        format++;
    }
    *buffer = '\0';
    return (buffer - (limit - bufsz));
}

#define _PRINT_BUF_SIZE (128)
static char _print_buf[_PRINT_BUF_SIZE];

int xsnprintf(char *buffer, size_t bufsz, char const *fmt, ...) 
{
    va_list val;
    va_start(val, fmt);
    int const rv = _VSNPRINTF(buffer, bufsz, fmt, val);
    va_end(val);
    return(rv);
}

int xvprintf(char const *fmt, va_list val)
{
    int const rv = _VSNPRINTF(_print_buf, sizeof(_print_buf), fmt, val);

    OS_PRINT_MSG(_print_buf);
    return(rv);
}

int xprintf(char const *fmt, ...)
{
    va_list val;
    va_start(val, fmt);
    int const rv = _VSNPRINTF(_print_buf, sizeof(_print_buf), fmt, val);
    va_end(val);

    OS_PRINT_MSG(_print_buf);
    return(rv);
}

int xvsnprintf(char *buffer, size_t bufsz, char const *format, va_list val)
{
    return _VSNPRINTF(buffer, bufsz, format, val);
}

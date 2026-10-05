/**
 * @file xprintf.h
 * @author Tropic Square
 * @brief Minimal printf implementation header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef XPRINTF_H
#define XPRINTF_H

#include <stddef.h>
#include <stdarg.h>
#include "type.h"

int xsnprintf(char *buffer, size_t bufsz, const char *fmt, ...) TS_CHECK_RETVAL;
int xvprintf(char const *fmt, va_list val) TS_CHECK_RETVAL;
int xprintf(char const *fmt, ...) TS_CHECK_RETVAL;
int xvsnprintf(char *buffer, size_t bufsz, char const *format, va_list val) TS_CHECK_RETVAL;

#endif // ! XPRINTF_H

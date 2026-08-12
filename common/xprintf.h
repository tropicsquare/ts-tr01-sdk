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

int xsnprintf(char *buffer, size_t bufsz, const char *fmt, ...);
int xvprintf(char const *fmt, va_list val);
int xprintf(char const *fmt, ...);
int xvsnprintf(char *buffer, size_t bufsz, char const *format, va_list val);

#endif // ! XPRINTF_H

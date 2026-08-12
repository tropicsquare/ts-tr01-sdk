/**
 * @file type.h
 * @author Tropic Square
 * @brief Basic types definition header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef char        ascii;

typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

/**
 * @brief More complex true/false types for security hardening.
 * 
 */
typedef enum {
    TS_TRUE     = 0xAA,
    TS_FALSE    = 0x55
} ts_bool; 

#define KB (1024)
#define MB (1024*KB)

#endif // ! TYPE_H

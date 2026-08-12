/**
 * @file ts_rd_bool.h
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief Redundant boolean type definition.
 *
 * This header defines a hardened boolean type using redundant words to
 * increase robustness against fault injection and memory corruption.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef TS_RD_BOOL_H
#define TS_RD_BOOL_H


/**
 * @brief Redundant boolean value for security hardening.
 * 
 */
typedef struct {
    u32 w1;
    u32 w2;
} ts_rd_bool;

#define TS_RD_TRUE_W1 (0x0b5e55ed)
#define TS_RD_TRUE_W2 (0xba11fade)
extern const ts_rd_bool TS_RD_TRUE;

#define TS_RD_FALSE_W1 (0xf4a1aa12)
#define TS_RD_FALSE_W2 (0x45ee0521)
extern const ts_rd_bool TS_RD_FALSE;

/**
 * @def TS_RD_SECURE_CALL
 * @brief Execute a function returning ::ts_rd_bool with integrity checking.
 *
 * @param func_call  Function call expression returning ::ts_rd_bool
 * @param fail_body  Code to execute on validation failure (e.g. error handling,
 *                   abort, return, or alarm)
 */
#define TS_RD_SECURE_CALL(func_call, fail_body) \
    do {                                        \
        volatile ts_rd_bool ret = TS_RD_FALSE;  \
        ret = func_call;                        \
        if (ret.w1 != TS_RD_TRUE.w1) {          \
            fail_body;                          \
        }                                       \
        if (ret.w2 != TS_RD_TRUE.w2) {          \
            fail_body;                          \
        }                                       \
    } while (0)


/**
 * @brief Check value is valid ts_rd_bool content.
 *
 * Make os_alarm() when value is not TS_RD_TRUE or TS_RD_FALSE
 */
void ts_rd_bool_assert_valid(ts_rd_bool value);

#endif // ! TS_RD_BOOL_H

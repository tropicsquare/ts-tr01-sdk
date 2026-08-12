/**
 * @file ftc.h
 * @author Tropic Square
 * @brief FTC (Firmware Testbench Channel) driver header file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef FTC_H
#define FTC_H

#if TS_SIMULATION_BUILD == 1

#include "type.h"
#include "io_ops.h"
#include "ftc_regs.h"

#define FTC_BASE_ADDRESS 0xFFFFFF00

#define FTC_TEST_PASSED_VALUE 0xFEEDBEEFU
#define FTC_TEST_FAILED_VALUE 0xDEADBEEFU

#define FTC_MSG_FLUSH_VALUE 0xBAAAAAADU
#define FTC_MSG_PRINT_VALUE 0xBEBEBEBEU

/**
 * @brief Signals successful end of FW operation to TB.
 */
void ftc_test_passed(void);

/**
 * @brief Signals error end of FW operation to TB.
 */
void ftc_test_failed(void);

/**
 * @brief Sends parameter to TB
 *
 * @param index Parameter index (0-3)
 * @param value Value of parameter to send.
 */
void ftc_send_to_tb(size_t index, u32 value);

/**
 * @brief Receives parameter from TB
 *
 * @param index Parameter index (0-3)
 * @returns Value of parameter received from TB
 */
u32 ftc_receive_from_tb(size_t index);

/**
 * @brief Prints formatted messages in TB simulation log.
 *
 * @param fmt String with basic format specifiers (see note).
 * @param ... Optional list of variables to be printed according to fmt.
 * 
 * @note For now the function supports %c, %s, %x, %u and %d (without support negative values).
 * The implementation is very minimalistic but it should suffice for basic register
 * value checking.
 * 
 * @note There is no specific limitation for fmt string length, because all processed characters
 * are directly output to the print register.
 */
void ftc_print_msg(const char *fmt, ...);

/**
 * @brief Fetch random number from TB.
 *
 * @returns Value of random number from TB
 */
u32 ftc_get_rnd_num (void);

static inline void ftc_put_char(const char ch)
{
    PTR32_T(FTC_BASE_ADDRESS + FTC_MSG_BUF_IN_ADDR) = (u32)ch;
}

static inline void ftc_print(void)
{
    PTR32_T(FTC_BASE_ADDRESS + FTC_MSG_BUF_CTRL_ADDR) = FTC_MSG_PRINT_VALUE;
}

/**
 * @todo Add functions for passing whole array of parameters in/out!
 * 
 */
///@{
#define FTC_TEST_PASSED     ftc_test_passed
#define FTC_TEST_FAILED     ftc_test_failed
#define FTC_SEND_TO_TB      ftc_send_to_tb
#define FTC_RECEIVE_FROM_TB ftc_receive_from_tb
#define FTC_PRINT_MSG       ftc_print_msg
#define FTC_GET_RND_NUM     ftc_get_rnd_num
#define FTC_PUT_CHAR        ftc_put_char
#define FTC_PRINT           ftc_print
///@}

#else // TS_SIMULATION_BUILD

#define FTC_TEST_PASSED( ... )
#define FTC_TEST_FAILED( ... )
#define FTC_SEND_TO_TB( ... )
#define FTC_RECEIVE_FROM_TB( ... )
#define FTC_PRINT_MSG( ... )
#define FTC_GET_RND_NUM( ... )
#define FTC_PUT_CHAR( ... )
#define FTC_PRINT( ... )

#endif // not TS_SIMULATION_BUILD

#endif // ! FTC_H

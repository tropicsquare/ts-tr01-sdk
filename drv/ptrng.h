/**
 * @file ptrng.h
 * @brief PTRNG driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 *
 * @author: Tropic Square
 */

#ifndef PTRNG_H
#define PTRNG_H

#include "type.h"
#include "tassic_defs.h"

typedef enum {
    PTRNG0 = BDRK_PTRNG0_BASE_ADDR,
    PTRNG1 = BDRK_PTRNG1_BASE_ADDR
} ptrng_address_e;

/**
 * @brief Enable bedrock and bedrock proxy clock needed for PTRNG function.
 *
 * Basic initialization to IP get working for tests.
 */
void ptrng_init(void);

/**
 * @brief From application initialization 
 *
 * Does specific initialization to get it working in app.
 * It expect all tests already passed during boot process and PTRNG is ready.
 */
void ptrng_init_app(void);

/**
 * @brief Turn ON selected PTRNG and performs the initialization and startup procedure then turn off.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant.
 * @param public_parameter A 32 bit value uniformly distributed accross different chips.
 *        This parameter is needed for security reasons to diversify the post-processing algorithm of the PTRNG for different chips.
 * @returns TS_TRUE - if the startup procedure of the PTRNG was successful,
 *          if TS_FALSE is returned, the PTRNG ouput cannot be used.
 */
ts_bool ptrng_startup_test(ptrng_address_e ptrng, u32 public_parameter) TS_CHECK_RETVAL;

/**
 * @brief Turns on selected PTRNG without startup procedure.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant.
 */
void ptrng_wakeup(ptrng_address_e ptrng);

/**
 * @brief Turns off the PTRNG clock.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant.
 */
void ptrng_suspend(ptrng_address_e ptrng);

/**
 * @brief Fetches the true random number output of the PTRNG to the buffer.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant.
 * @param dest the destination buffer
 * @param len number of bytes to fetch
 * @returns TS_TRUE - if no alarm of the online tests has been raised,
 *          TS_FALSE if an alarm has been raised - in such scenario, the output of the PTRNG cannot be used.
 */
ts_bool ptrng_read(ptrng_address_e ptrng, u8 *dest, size_t len) TS_CHECK_RETVAL;

/**
 * @brief Sets the T2D data access mode.
 *
 * @param ptrng PTRNG0 or PTRNG1 constant.
 * @param enable TS_TRUE for enabling the T2D data mode, TS_FALSE for disabling
 * (this is the normal PTRNG output mode)
 */

void ptrng_t2d_data_mode(ptrng_address_e ptrng, ts_bool enable);

#endif // ! PTRNG_H

/**
 * @file mbist.h
 * @brief Driver for TSMBIST engine for TROPIC01
 * @author Tropic Square
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef MBIST_H
#define MBIST_H

#include <type.h>

/**
 * @enum mbist_chnls_e
 * @brief MBIST channel mapping according to TROPIC01 functional specification (section 25).
 */
typedef enum {
    MBIST_CHN_CPUSS_DRAM        = (1 << 0),
    MBIST_CHN_CPUSS_IRAM        = (1 << 1),
    MBIST_CHN_CPUSS_IROM        = (1 << 2),
    MBIST_CHN_SS_REQQ           = (1 << 3),
    MBIST_CHN_SS_RSPQ           = (1 << 4),
    MBIST_CHN_SPECT_IMEM        = (1 << 5),
    MBIST_CHN_SPECT_DRAM_IN     = (1 << 6),
    MBIST_CHN_SPECT_DRAM_OUT    = (1 << 7),
    MBIST_CHN_SPECT_CONST_ROM   = (1 << 8),
    MBIST_CHN_SPECT_RF_0        = (1 << 9),
    MBIST_CHN_SPECT_RF_1        = (1 << 10),
    MBIST_CHN_CPB_DESC_MEM      = (1 << 11),
    MBIST_CHN_CPB_CMD_BUF       = (1 << 12),
    MBIST_CHN_CPB_RES_BUF       = (1 << 13),
    MBIST_CHN_FSS_RAM_BUF       = (1 << 14)
} mbist_chnls_e;


/**
 * @typedef mbist_chnls_t
 * @brief Bitmask for specifying one or more MBIST channels.
 */
typedef u32 mbist_chnls_t;


/**
 * @name MBIST Channel Group Definitions
 * @brief Predefined sets of MBIST channels for convenience.
 * @{
 */
#define MBIST_CHANNELS_CPU_RAMS (                       \
    MBIST_CHN_CPUSS_IRAM       |                        \
    MBIST_CHN_CPUSS_DRAM                                \
)

#define MBIST_CHANNELS_CPU_ROMS (                       \
    MBIST_CHN_CPUSS_IROM                                \
)

#define MBIST_CHANNELS_PERIPH_RAMS (                    \
    MBIST_CHN_SS_REQQ          |                        \
    MBIST_CHN_SS_RSPQ          |                        \
    MBIST_CHN_SPECT_IMEM       |                        \
    MBIST_CHN_SPECT_DRAM_IN    |                        \
    MBIST_CHN_SPECT_DRAM_OUT   |                        \
    MBIST_CHN_SPECT_RF_0       |                        \
    MBIST_CHN_SPECT_RF_1       |                        \
    MBIST_CHN_CPB_DESC_MEM     |                        \
    MBIST_CHN_CPB_CMD_BUF      |                        \
    MBIST_CHN_CPB_RES_BUF      |                        \
    MBIST_CHN_FSS_RAM_BUF                               \
)

#define MBIST_CHANNELS_PERIPH_ROMS (                    \
    MBIST_CHN_SPECT_CONST_ROM                           \
)

#define MBIST_CHANNEL_ALL_RAMS (MBIST_CHANNELS_CPU_RAMS | MBIST_CHANNELS_PERIPH_RAMS)
#define MBIST_CHANNEL_ALL_ROMS (MBIST_CHANNELS_CPU_ROMS | MBIST_CHANNELS_PERIPH_ROMS)
#define MBIST_CHANNELS_ALL (MBIST_CHANNEL_ALL_RAMS | MBIST_CHANNEL_ALL_ROMS)
/** @} */

/**
 * @enum mbist_mode_t
 * @brief MBIST operation mode.
 */
typedef enum {
    MBIST_MODE_MANUFACTURING,
    MBIST_MODE_FAULT_ANALYSIS
} mbist_mode_t;

/**
 * @enum mbist_test_type_e
 * @brief Test type algorithms.
 */
typedef enum {
    MBIST_TEST_MARCH_X  = 0,
    MBIST_TEST_MARCH_C  = 1,
    MBIST_TEST_MARCH_SS = 2,
    MBIST_TEST_CRC      = 3,
    MBIST_TEST_MEM_CLR  = 4
} mbist_test_type_e;

/**
 * @struct mbist_config_t
 * @brief Configuration structure for MBIST engine for test init.
 */
typedef struct {
    u8              pattern_a;
    u8              pattern_b;
    u32             retention_1;
    u32             retention_2;
} mbist_config_t;

/**
 * @brief Initializes the MBIST engine.
 *
 * Resets the engine, sets up the retention timing and the patterns and enables
 * the DONE interrupt, which the driver needs to detect the end of a test.
 *
 * Has to be called before mbist_exec_test(). mbist_erase() enables the DONE
 * interrupt itself, so it works without this call.
 *
 * @param config Pointer to the MBIST configuration structure.
 * @note The engine is left woken up, ready for mbist_exec_test(). The caller
 *       suspends it with mbist_suspend() once it is done with the tests.
 */
void mbist_init(const mbist_config_t *config);

/**
 * @brief Suspends the MBIST engine.
 * Ddisable clock for TSMBIST
 */
void mbist_suspend(void);

/**
 * @brief Wakes up the MBIST engine.
 * Re-enables the clock to the TSMBIST engine.
 */
void mbist_wakeup(void);

/**
 * @brief Executes an MBIST test on the specified memory channels.
 *
 * Configures the test type, prepares the engine, starts the test and blocks
 * until it finishes. The tested memories are taken over from the application for
 * the duration of the test and released before the call returns.
 *
 * Only the MARCH tests and MEM_CLR are evaluated - the result is taken from the
 * TEST_PROGRESS, TEST_RESULT and TEST_ERROR registers, which TSMBIST does not
 * maintain for MBIST_TEST_CRC. A CRC test would need its outcome read from the
 * CRC_RESULT register and compared by the caller.
 *
 * @param test_type The test pattern to apply (see @ref mbist_test_type_e).
 * @param chnls     Bitmask of memory channels to test.
 * @return TS_TRUE if all the channels passed.  
 *         TS_FALSE otherwise, including a test which did not finish in time and
 *         a corrupted or unresponsive engine which could not be prepared.
 * @note The engine has to be awake and set up by mbist_init(), and it is left
 *       awake - the caller suspends it with mbist_suspend() when done with the
 *       tests. Unlike mbist_erase(), this call manages no clock of its own.
 * @note The engine is not reset before the test, so a test which is already in
 *       progress is not aborted - the preparation fails instead. mbist_init()
 *       brings the engine into a known state.
 * @note The tested memories must not be in use by the application, as the engine
 *       takes them over - testing a memory the CPU runs from hangs the CPU.
 * @note MBIST_TEST_CRC is rejected with TS_FALSE, as its outcome cannot be
 *       evaluated from the result registers.
 */
ts_bool mbist_exec_test(mbist_test_type_e test_type, mbist_chnls_t chnls) TS_CHECK_RETVAL;

/**
 * @brief Executes a full MBIST erase cycle on the specified memory channels.
 * This function wraps wake-up, initialization, memory clear test, and suspension.
 * @param chnls Bitmask of memory channels to be erased.
 * @return TS_TRUE if erase was successful.
 *         TS_FALSE otherwise, including an erase which did not finish in time.
 * @note The outcome is taken from the TEST_PROGRESS, TEST_RESULT and TEST_ERROR
 *       registers, the same way as for mbist_exec_test().
 * @note The engine is reset before the erase, which aborts any test in progress -
 *       the erase of the sensitive data takes priority over anything running.
 *       The setup passed to mbist_init() is kept.
 * @note The engine is woken up by this call and suspended again on the end, so
 *       it needs no mbist_init(). The MBIST clock is left disabled even when the
 *       caller had it enabled, so an erase between mbist_init() and a test makes
 *       the test fail - mbist_wakeup() is needed to continue testing.
 */
ts_bool mbist_erase(mbist_chnls_t chnls) TS_CHECK_RETVAL;

#endif


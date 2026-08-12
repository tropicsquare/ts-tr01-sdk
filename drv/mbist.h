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


#define MBIST_RES_OK 0
#define MBIST_RES_FAIL 0xFFFFFFFF

/**
 * @brief Initializes the MBIST engine.
 *
 * Sets up retention timing and patterns. If a test is already in progress,
 * the engine is reset.
 *
 * @param config Pointer to the MBIST configuration structure.
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
 * @brief Prepares the MBIST engine for test execution.
 *
 * Configures the memory test type and the memory channels to be tested.
 *
 * @param test_type The test pattern to apply (see @ref mbist_test_type_e).
 * @param chnls     Bitmask of memory channels to be tested.
 * @return MBIST_RES_OK if the configuration was successful.  
 *         MBIST_RES_FAIL if the TSMBIST engine is corrupted or unresponsive.
 */
u32 mbist_prepare_test(mbist_test_type_e test_type, mbist_chnls_t chnls);


/**
 * @brief Executes the configured MBIST test.
 *
 * Starts the memory test on specified channels and blocks until completion.
 *
 * @param chnls Bitmask of memory channels to test.
 * @return MBIST_RES_OK if all tests passed.  
 *         Bitmask of failed channels otherwise.
 */
u32 mbist_exec_test(mbist_chnls_t chnls);

/**
 * @brief Executes a full MBIST erase cycle on the specified memory channels.
 * This function wraps wake-up, initialization, memory clear test, and suspension.
 * @param chnls Bitmask of memory channels to be erased.
 */
void mbist_erase(mbist_chnls_t chnls);

#endif


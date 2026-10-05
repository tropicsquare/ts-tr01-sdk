/**
 * @file mbist.c
 * @brief Driver for TSMBIST engine for TROPIC01
 * @author Tropic Square
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "io_ops.h"
#include "os.h"

#include "tassic_defs.h"
#include "mbist_regs.h"
#include "soc_ctrl.h"
#include "hw.h"

#include "mbist.h"

#define _MBIST_REG_READ(offset)             IO_READ_32(TROPIC01_MEMORY_MAP_MBIST_BASE_ADDR+(offset))
#define _MBIST_REG_WRITE(offset,value)      IO_WRITE_32(TROPIC01_MEMORY_MAP_MBIST_BASE_ADDR+(offset), value)

/**
 * @brief Depth of the deepest memory connected to an MBIST channel [words].
 * Tests run on all selected channels in parallel, so the test duration is given
 * by the deepest tested memory - CPUSS Instruction RAM with 24 kB.
 */
#define _MBIST_MAX_MEM_DEPTH   (6144)

/**
 * @brief Number of MBIST clock cycles spent on one memory word.
 * The longest test is MARCH SS, which takes 71 cycles per word in MANUFACTURING
 * MODE: 2 for the initial write row, 16 for each of the four 5-operation rows
 * and 5 for the final read row (state chains of the TSMBIST controller FSM,
 * every state takes one cycle).
 */
#define _MBIST_CYCLES_PER_WORD (71)

/** @brief Safety margin added to the computed test duration, 2 ms at 70 MHz. */
#define _MBIST_MARGIN_CYCLES   (140000)

/** @brief Worst-case test duration in MBIST clock cycles, retention time excluded. */
#define _MBIST_MAX_TEST_CYCLES ((_MBIST_MAX_MEM_DEPTH * _MBIST_CYCLES_PER_WORD) \
                                + _MBIST_MARGIN_CYCLES)

static volatile ts_bool command_in_progress;

/**
 * @brief Resets the engine, optionally keeping the test setup untouched.
 *
 * The reset clears all registers, so the setup is read out before the reset and
 * written back afterwards. The following registers are restored:
 * - CONFIG[MODE], PATTERN, RETENTION_1, RETENTION_2 - the setup not bound to a
 *   single test,
 * - INT_EN - managed by the driver, DONE is needed to detect the test end.
 *
 * The rest is left in reset values on purpose:
 * - CONFIG[MBIST_EN] - the engine must not take the tested memories over again,
 * - CONFIG[TEST_TYPE], MEM_SEL - set per test by mbist_prepare_test() / erase,
 * - CONFIG[DATA_INDEX] - indexes the result registers cleared by the reset.
 *
 * @param restore_config TS_TRUE to keep the current setup, TS_FALSE to leave all
 *                       the registers in their reset values.
 * @note The reset is deasserted internally with a delay, so the engine must not
 *       be accessed for at least 5 cycles of its clock afterwards.
 */
static void _mbist_reset(ts_bool restore_config)
{
    u32 mode        = _MBIST_REG_READ(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MODE_MASK;
    u32 int_en      = _MBIST_REG_READ(MBIST_INT_EN_ADDR);
    u32 pattern     = _MBIST_REG_READ(MBIST_PATTERN_ADDR);
    u32 retention_1 = _MBIST_REG_READ(MBIST_RETENTION_1_ADDR);
    u32 retention_2 = _MBIST_REG_READ(MBIST_RETENTION_2_ADDR);

    command_in_progress = TS_FALSE;
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_RST_MASK);
    os_delay_cycles(5); // Required by MBIST specification.

    if (restore_config == TS_TRUE)
    {
        _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, mode);
        _MBIST_REG_WRITE(MBIST_INT_EN_ADDR, int_en);
        _MBIST_REG_WRITE(MBIST_PATTERN_ADDR, pattern);
        _MBIST_REG_WRITE(MBIST_RETENTION_1_ADDR, retention_1);
        _MBIST_REG_WRITE(MBIST_RETENTION_2_ADDR, retention_2);
    }
}

static TS_CHECK_RETVAL ts_bool _mbist_cmd_done(void)
{
    return (command_in_progress == TS_FALSE) ? TS_TRUE : TS_FALSE;
}

void mbist_init(const mbist_config_t *config)
{
    OS_SANITY_NULL(config);

    mbist_wakeup();
    _mbist_reset(TS_FALSE);  // the whole setup is written below

    // Configure
    _MBIST_REG_WRITE(MBIST_RETENTION_1_ADDR, config->retention_1);
    _MBIST_REG_WRITE(MBIST_RETENTION_2_ADDR, config->retention_2);

    u32 pattern = FIELD_PREP(MBIST_PATTERN_A_MASK, config->pattern_a) |
                  FIELD_PREP(MBIST_PATTERN_B_MASK, config->pattern_b);
    _MBIST_REG_WRITE(MBIST_PATTERN_ADDR, pattern);

    _MBIST_REG_WRITE(MBIST_INT_EN_ADDR, MBIST_INT_EN_DONE_EN_MASK);

    // TODO: Fault analysis mode support

    // Keep disabled until test executed
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR,  0);
}

void mbist_suspend(void)
{
    soc_ctrl_clk_dis(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK);
}

void mbist_wakeup(void)
{
    soc_ctrl_clk_en(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK);
}

/**
 * @brief Selects the channels and prepares the engine for the test.
 *
 * The engine leaves its idle state only with CONFIG[MBIST_EN] set, so the bit is
 * set here - the PREPARE command is ignored without it. MEM_SEL is written first
 * on purpose: the mbist_en signal reflects MEM_SEL as soon as the engine is
 * enabled, so a selection left by a previous test would hand the wrong memories
 * over to the engine.
 *
 * The engine stays enabled for the test which follows, which is what keeps the
 * tested memories taken over from the application - they are released by the
 * engine reset which _mbist_exec() does when the sequence fails.
 *
 * @param chnls Bitmask of the channels to be tested.
 * @return TS_TRUE when the engine reports the test prepared on all the selected
 *         channels (see TSMBIST design spec, use cases 8.1 and 8.2).
 */
static TS_CHECK_RETVAL ts_bool _mbist_prepare(mbist_chnls_t chnls)
{
    _MBIST_REG_WRITE(MBIST_MEM_SEL_ADDR, chnls);
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR,
                     _MBIST_REG_READ(MBIST_CONFIG_ADDR) | MBIST_CONFIG_MBIST_EN_MASK);
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_PREPARE_MASK);

    if ((_MBIST_REG_READ(MBIST_TEST_PROGRESS_ADDR) & chnls) != chnls)
    {
        return TS_FALSE;
    }
    if (_MBIST_REG_READ(MBIST_TEST_ERROR_ADDR) & chnls)
    {
        return TS_FALSE;
    }
    if (_MBIST_REG_READ(MBIST_TEST_RESULT_ADDR) & chnls)
    {
        return TS_FALSE;
    }
    u32 sts_reg = _MBIST_REG_READ(MBIST_STATUS_ADDR);
    if (sts_reg & MBIST_STATUS_DONE_MASK)
    {
        return TS_FALSE;
    }
    return TS_TRUE;
}

/**
 * @brief Computes the wait time for the test to finish.
 *
 * The worst case is used for all the tests to keep the logic simple: MARCH SS
 * (the longest test) on the deepest memory, including both retention waits.
 *
 * @return Timeout in us.
 * @note Assumes the MBIST clock runs at HW_CLOCK_MHZ.
 */
static TS_CHECK_RETVAL u32 _mbist_get_timeout_us(void)
{
    // each part is converted separately to prevent overflow of the cycles sum
    u32 timeout_us = _MBIST_MAX_TEST_CYCLES / HW_CLOCK_MHZ;

    timeout_us += _MBIST_REG_READ(MBIST_RETENTION_1_ADDR) / HW_CLOCK_MHZ;
    timeout_us += _MBIST_REG_READ(MBIST_RETENTION_2_ADDR) / HW_CLOCK_MHZ;

    return timeout_us;
}

/**
 * @brief Evaluates the result of a finished test.
 *
 * A channel passed only when its test is not in progress, its result is set and
 * no error was detected on it (see TSMBIST design spec, use cases 8.1 and 8.2).
 *
 * @param chnls Bitmask of the channels the test ran on.
 * @return TS_TRUE when all the channels passed.
 * @note The results are valid only for a test which reached its end, and only for
 *       the MARCH tests and MEM_CLR - TSMBIST does not maintain the registers for
 *       MBIST_TEST_CRC.
 * @note The results are cleared by the engine reset, so they have to be read
 *       before it.
 */
static TS_CHECK_RETVAL ts_bool _mbist_test_passed(mbist_chnls_t chnls)
{
    u32 failed_chnls = (_MBIST_REG_READ(MBIST_TEST_PROGRESS_ADDR)
                     | (~_MBIST_REG_READ(MBIST_TEST_RESULT_ADDR))
                     | _MBIST_REG_READ(MBIST_TEST_ERROR_ADDR)) & chnls;

    return (failed_chnls == 0) ? TS_TRUE : TS_FALSE;
}

/**
 * @brief Starts the prepared test and waits for its end.
 *
 * The result is evaluated here, while the engine still holds it - the reset which
 * _mbist_exec() does on a failure clears the result registers.
 *
 * @param chnls Bitmask of the channels the test runs on.
 * @return TS_TRUE when the test finished in time and all the channels passed.
 */
static TS_CHECK_RETVAL ts_bool _mbist_run(mbist_chnls_t chnls)
{
    command_in_progress = TS_TRUE;
    _MBIST_REG_WRITE(MBIST_COMMAND_ADDR, MBIST_COMMAND_START_MASK);

    if (os_wait_for(_mbist_cmd_done, _mbist_get_timeout_us()) != TS_TRUE)
    {   // the results are meaningless for a test which did not reach its end
        return TS_FALSE;
    }

    return _mbist_test_passed(chnls);
}

/**
 * @brief Executes the test configured in CONFIG on the given channels.
 *
 * @param chnls Bitmask of the channels to be tested.
 * @return TS_TRUE when the engine was prepared, the test finished in time and
 *         all the channels passed.
 * @note The engine is left reset on a failure, so it holds no tested memory. The
 *       caller restores the CONFIG register in both cases.
 */
static TS_CHECK_RETVAL ts_bool _mbist_exec(mbist_chnls_t chnls)
{
    ts_bool ret = _mbist_prepare(chnls);
    if (ret == TS_TRUE)
    {
        ret = _mbist_run(chnls);
    }

    if (ret != TS_TRUE)
    {   // stop a test which possibly still runs before the engine is released
        _mbist_reset(TS_TRUE);
    }
    return ret;
}

ts_bool mbist_exec_test(mbist_test_type_e test_type, mbist_chnls_t chnls)
{
    if (test_type == MBIST_TEST_CRC)
    {   // TSMBIST maintains no result registers for it, so it cannot be evaluated
        return TS_FALSE;
    }

    // the engine is enabled by the preparation, once the channels are selected
    u32 cfg_reg = _MBIST_REG_READ(MBIST_CONFIG_ADDR) & ~MBIST_CONFIG_MBIST_EN_MASK;
    FIELD_SET(cfg_reg, MBIST_CONFIG_TEST_TYPE_MASK, test_type);

    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg);

    ts_bool ret = _mbist_exec(chnls);

    // restore CFG to the disabled state
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg);

    return ret;
}

ts_bool mbist_erase(mbist_chnls_t chnls)
{
    mbist_wakeup();

    // erase runs with its own setup, so the current one is restored on the end
    u32 cfg_reg = _MBIST_REG_READ(MBIST_CONFIG_ADDR) & ~MBIST_CONFIG_MBIST_EN_MASK;

    _mbist_reset(TS_TRUE);

    // keep the interrupts already enabled, DONE is needed to detect the end
    _MBIST_REG_WRITE(MBIST_INT_EN_ADDR, _MBIST_REG_READ(MBIST_INT_EN_ADDR) | MBIST_INT_EN_DONE_EN_MASK);
    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, MBIST_TEST_MEM_CLR << MBIST_CONFIG_TEST_TYPE_POS);

    ts_bool ret = _mbist_exec(chnls);

    _MBIST_REG_WRITE(MBIST_CONFIG_ADDR, cfg_reg);
    mbist_suspend();
    return ret;
}


__ISR void irq_mbist_handler(void)
{
    u32 sts_reg = _MBIST_REG_READ(MBIST_STATUS_ADDR);
    _MBIST_REG_WRITE(MBIST_STATUS_ADDR, sts_reg);

    if (sts_reg & MBIST_STATUS_DONE_MASK)
    {
        command_in_progress = TS_FALSE;
    }
}

/**
 * @file test_mbist.c
 * @brief Unit tests for the MBIST driver - drv/mbist.c
 *
 * The driver talks to the TSMBIST engine through memory mapped registers only,
 * so the tests run against a behavioral model of the engine. The io_ops.h stub
 * routes every register access to mmio_read_32() / mmio_write_32() implemented
 * below, which keep the register storage and apply the side effects specified in
 * ODS_TSMBIST_design_spec: W1C on STATUS, W1S on COMMAND, a reset which clears
 * all the registers, PREPARE publishing MEM_SEL in TEST_PROGRESS and START
 * running a test which ends by calling irq_mbist_handler().
 *
 * What the engine does is scriptable through the "model" structure, so both a
 * healthy engine and the failure modes (an unresponsive engine, a channel
 * reporting an error, a test which never leaves TEST_PROGRESS) are covered.
 *
 * Every register write, OS call and clock switch is recorded in one event log,
 * which is what makes the ordering assertions possible - e.g. that the engine is
 * reset BEFORE its clock is disabled.
 *
 * The static functions of the driver (_mbist_reset(), _mbist_prepare(),
 * _mbist_run(), _mbist_cmd_done(), _mbist_get_timeout_us()) are covered through
 * the public API, which reaches all of their paths.
 *
 * @note mbist_init() sanity checks its argument with OS_SANITY_NULL, which maps
 *       to assert() in the host build and aborts the process, so the NULL case
 *       is not covered here.
 */

#include "unity.h"

#include "mbist.h"
#include "mbist_regs.h"
#include "tassic_defs.h"
#include "hw.h"

#include "mock_os.h"
#include "mock_soc_ctrl.h"

/** @brief Interrupt handler of the driver, called by the model to signal DONE. */
void irq_mbist_handler(void);

#define _BASE           TROPIC01_MEMORY_MAP_MBIST_BASE_ADDR
#define _REG_SPAN       (0x40)
#define _REG_COUNT      (_REG_SPAN / sizeof(u32))

/* Reset values taken from the register map of the design specification. */
#define _BLOCK_ID_RESET (0x00000320)
#define _PATTERN_RESET  (0x0000AA55)

/* Channels used by the tests: a set and one channel outside of it. */
#define _CHNLS          (MBIST_CHN_SPECT_DRAM_IN | MBIST_CHN_CPB_CMD_BUF)
#define _CHNL_OTHER     (MBIST_CHN_CPUSS_IRAM)

/**
 * @brief Worst-case test duration the driver is expected to wait for.
 * Mirrors the constants of the driver: MARCH SS takes 71 MBIST clock cycles per
 * word on the deepest memory channel (CPUSS I-RAM, 6144 words), plus the safety
 * margin of 2 ms at 70 MHz.
 */
#define _EXPECTED_BASE_US (((6144u * 71u) + 140000u) / HW_CLOCK_MHZ)

/***************************************************************************************************
*   Event log
***************************************************************************************************/

typedef enum {
    EV_WRITE,       /**< register write: a = offset, b = value */
    EV_DELAY,       /**< os_delay_cycles(): a = cycles */
    EV_WAIT,        /**< os_wait_for(): a = timeout in us */
    EV_CLK_EN,      /**< soc_ctrl_clk_en(): a = peripheral mask */
    EV_CLK_DIS      /**< soc_ctrl_clk_dis(): a = peripheral mask */
} ev_kind_t;

typedef struct {
    ev_kind_t kind;
    u32       a;
    u32       b;
} event_t;

static event_t _events[128];
static size_t  _event_count;

static void _log(ev_kind_t kind, u32 a, u32 b)
{
    TEST_ASSERT_LESS_THAN_MESSAGE(sizeof(_events) / sizeof(_events[0]), _event_count,
                                  "event log overflow");
    _events[_event_count].kind = kind;
    _events[_event_count].a    = a;
    _events[_event_count].b    = b;
    _event_count++;
}

/** @brief Index of the first event of the given kind, or -1. */
static int _find(ev_kind_t kind, size_t from)
{
    for (size_t i = from; i < _event_count; i++)
    {
        if (_events[i].kind == kind)
        {
            return (int)i;
        }
    }
    return -1;
}

/** @brief Index of the first write to the given register offset, or -1. */
static int _find_write(u32 offset, size_t from)
{
    for (size_t i = from; i < _event_count; i++)
    {
        if ((_events[i].kind == EV_WRITE) && (_events[i].a == offset))
        {
            return (int)i;
        }
    }
    return -1;
}

/** @brief Index of the first COMMAND write carrying any of the given bits, or -1. */
static int _find_command_from(u32 mask, size_t from)
{
    for (size_t i = from; i < _event_count; i++)
    {
        if ((_events[i].kind == EV_WRITE) && (_events[i].a == MBIST_COMMAND_ADDR)
            && ((_events[i].b & mask) != 0))
        {
            return (int)i;
        }
    }
    return -1;
}

static int _find_command(u32 mask)
{
    return _find_command_from(mask, 0);
}

/***************************************************************************************************
*   TSMBIST model
***************************************************************************************************/

static u32 _regs[_REG_COUNT];

static struct {
    ts_bool prepare_sets_progress;  /**< PREPARE publishes MEM_SEL in TEST_PROGRESS */
    u32     prepare_forces_error;   /**< TEST_ERROR bits left set by PREPARE */
    u32     prepare_forces_result;  /**< TEST_RESULT bits left set by PREPARE */
    ts_bool prepare_keeps_done;     /**< STATUS[DONE] stays set after PREPARE */
    ts_bool run_finishes;           /**< the test ends, otherwise the engine hangs */
    u32     polls_before_done;      /**< condition polls before the test ends */
    u32     error_chnls;            /**< channels which report an error */
    ts_bool keeps_progress;         /**< TEST_PROGRESS stays set after the test */
    ts_bool result_despite_error;   /**< TEST_RESULT is set even for a failed channel */
    ts_bool signals_paused;         /**< STATUS[PAUSED] is set instead of STATUS[DONE] */
    u32     max_polls;              /**< polls the os_wait_for() stub performs */
} model;

static ts_bool _run_active;
static u32     _polls_left;

static void _regs_reset(void)
{
    memset(_regs, 0, sizeof(_regs));
    _regs[MBIST_BLOCK_ID_ADDR / sizeof(u32)] = _BLOCK_ID_RESET;
    _regs[MBIST_PATTERN_ADDR / sizeof(u32)]  = _PATTERN_RESET;
    _run_active = TS_FALSE;
}

/** @brief Register storage accessors for the test body, bypassing the model. */
static u32 _reg(u32 offset)
{
    return _regs[offset / sizeof(u32)];
}

static void _set_reg(u32 offset, u32 value)
{
    _regs[offset / sizeof(u32)] = value;
}

/** @brief End of the test - publishes the results and raises the interrupt. */
static void _engine_finish(void)
{
    u32 sel = _reg(MBIST_MEM_SEL_ADDR);

    _run_active = TS_FALSE;

    if (model.keeps_progress != TS_TRUE)
    {
        _regs[MBIST_TEST_PROGRESS_ADDR / sizeof(u32)] &= ~sel;
    }
    // TEST_RESULT is set only for the channels on which no error was detected
    u32 passed = (model.result_despite_error == TS_TRUE) ? sel : (sel & ~model.error_chnls);
    _regs[MBIST_TEST_RESULT_ADDR / sizeof(u32)] |= passed;
    _regs[MBIST_TEST_ERROR_ADDR / sizeof(u32)]  |= sel & model.error_chnls;

    if (model.signals_paused == TS_TRUE)
    {
        _regs[MBIST_STATUS_ADDR / sizeof(u32)] |= MBIST_STATUS_PAUSED_MASK;
    }
    else
    {
        _regs[MBIST_STATUS_ADDR / sizeof(u32)] |= MBIST_STATUS_DONE_MASK;
    }
    irq_mbist_handler();
}

/** @brief Advances the running test, called once per condition poll. */
static void _engine_tick(void)
{
    if (_run_active != TS_TRUE)
    {
        return;
    }
    if (_polls_left > 0)
    {
        _polls_left--;
    }
    if ((_polls_left == 0) && (model.run_finishes == TS_TRUE))
    {
        _engine_finish();
    }
}

static void _engine_prepare(void)
{
    u32 sel = _reg(MBIST_MEM_SEL_ADDR);

    if (model.prepare_sets_progress == TS_TRUE)
    {
        _regs[MBIST_TEST_PROGRESS_ADDR / sizeof(u32)] |= sel;
    }
    _regs[MBIST_TEST_RESULT_ADDR / sizeof(u32)] &= ~sel;
    _regs[MBIST_TEST_ERROR_ADDR / sizeof(u32)]  &= ~sel;
    _regs[MBIST_STATUS_ADDR / sizeof(u32)]      &= ~MBIST_STATUS_DONE_MASK;

    // failure modes of a corrupted engine
    _regs[MBIST_TEST_ERROR_ADDR / sizeof(u32)]  |= model.prepare_forces_error;
    _regs[MBIST_TEST_RESULT_ADDR / sizeof(u32)] |= model.prepare_forces_result;
    if (model.prepare_keeps_done == TS_TRUE)
    {
        _regs[MBIST_STATUS_ADDR / sizeof(u32)] |= MBIST_STATUS_DONE_MASK;
    }
}

static void _engine_command(u32 data)
{
    // the engine leaves its idle state only when CONFIG[MBIST_EN] is set
    ts_bool enabled = (_reg(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MBIST_EN_MASK) ? TS_TRUE : TS_FALSE;

    if (data & MBIST_COMMAND_RST_MASK)
    {
        _regs_reset();
    }
    if ((data & MBIST_COMMAND_PREPARE_MASK) && (enabled == TS_TRUE))
    {
        _engine_prepare();
    }
    if ((data & MBIST_COMMAND_START_MASK) && (enabled == TS_TRUE))
    {
        _run_active = TS_TRUE;
        _polls_left = model.polls_before_done;
        if (_polls_left == 0)
        {
            _engine_finish();
        }
    }
}

u32 mmio_read_32(u32 addr)
{
    TEST_ASSERT_TRUE_MESSAGE((addr >= _BASE) && (addr < (_BASE + _REG_SPAN)),
                             "read outside of the MBIST register space");
    return _regs[(addr - _BASE) / sizeof(u32)];
}

void mmio_write_32(u32 addr, u32 data)
{
    TEST_ASSERT_TRUE_MESSAGE((addr >= _BASE) && (addr < (_BASE + _REG_SPAN)),
                             "write outside of the MBIST register space");
    u32 offset = addr - _BASE;

    _log(EV_WRITE, offset, data);

    switch (offset)
    {
        case MBIST_STATUS_ADDR:                 // W1C
            _regs[offset / sizeof(u32)] &= ~data;
            break;

        case MBIST_COMMAND_ADDR:                // WO, self clearing
            _engine_command(data);
            break;

        case MBIST_BLOCK_ID_ADDR:               // RO
        case MBIST_TEST_PROGRESS_ADDR:
        case MBIST_TEST_RESULT_ADDR:
        case MBIST_TEST_ERROR_ADDR:
        case MBIST_CRC_RESULT_ADDR:
            break;

        default:
            _regs[offset / sizeof(u32)] = data;
            break;
    }
}

volatile u32 *mmio_ptr_32(u32 addr)
{
    TEST_ASSERT_TRUE_MESSAGE((addr >= _BASE) && (addr < (_BASE + _REG_SPAN)),
                             "access outside of the MBIST register space");
    return &_regs[(addr - _BASE) / sizeof(u32)];
}

/***************************************************************************************************
*   OS and clock stubs
***************************************************************************************************/

static ts_bool _wait_for_stub(os_wait_for_pfunc_t condition, u32 timeout_us, int num_calls)
{
    (void)num_calls;
    _log(EV_WAIT, timeout_us, 0);

    for (u32 i = 0; i <= model.max_polls; i++)
    {
        if (condition() == TS_TRUE)
        {
            return TS_TRUE;
        }
        _engine_tick();
    }
    return TS_FALSE;    // the condition was not met in time
}

static void _delay_cycles_stub(u32 cycles, int num_calls)
{
    (void)num_calls;
    _log(EV_DELAY, cycles, 0);
}

static void _clk_en_stub(soc_ctrl_periph_clk_en_t peripherals, int num_calls)
{
    (void)num_calls;
    _log(EV_CLK_EN, peripherals, 0);
}

static void _clk_dis_stub(soc_ctrl_periph_clk_en_t peripherals, int num_calls)
{
    (void)num_calls;
    _log(EV_CLK_DIS, peripherals, 0);
}

/***************************************************************************************************
*   Fixture
***************************************************************************************************/

/** @brief Configuration with values distinguishable from the reset ones. */
static const mbist_config_t CONFIG = {
    .pattern_a   = 0x12,
    .pattern_b   = 0x34,
    .retention_1 = 0x1000,
    .retention_2 = 0x2000
};

/** @brief PATTERN register value corresponding to CONFIG. */
#define _CONFIG_PATTERN (0x3412)

void setUp(void)
{
    _regs_reset();
    _event_count = 0;
    _polls_left  = 0;

    memset(&model, 0, sizeof(model));
    model.prepare_sets_progress = TS_TRUE;
    model.run_finishes          = TS_TRUE;
    model.polls_before_done     = 1;
    model.max_polls             = 8;

    os_wait_for_Stub(_wait_for_stub);
    os_delay_cycles_Stub(_delay_cycles_stub);
    soc_ctrl_clk_en_Stub(_clk_en_stub);
    soc_ctrl_clk_dis_Stub(_clk_dis_stub);
}

void tearDown(void) {}

/** @brief Brings the engine into the state left by a completed mbist_init(). */
static void _given_initialized(void)
{
    mbist_init(&CONFIG);
    _event_count = 0;
}

/***************************************************************************************************
*   mbist_wakeup() / mbist_suspend()
***************************************************************************************************/

void test_wakeup_enables_the_mbist_clock(void)
{
    mbist_wakeup();

    TEST_ASSERT_EQUAL_INT(0, _find(EV_CLK_EN, 0));
    TEST_ASSERT_EQUAL_HEX32(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK, _events[0].a);
}

void test_suspend_disables_the_mbist_clock(void)
{
    mbist_suspend();

    TEST_ASSERT_EQUAL_INT(0, _find(EV_CLK_DIS, 0));
    TEST_ASSERT_EQUAL_HEX32(SOC_CTRL_CLK_EN_MBISTCLKEN_MASK, _events[0].a);
}

/***************************************************************************************************
*   mbist_init()
***************************************************************************************************/

void test_init_wakes_the_engine_up_and_resets_it(void)
{
    mbist_init(&CONFIG);

    int clk = _find(EV_CLK_EN, 0);
    int rst = _find_command(MBIST_COMMAND_RST_MASK);

    TEST_ASSERT_GREATER_OR_EQUAL(0, clk);
    TEST_ASSERT_GREATER_OR_EQUAL(0, rst);
    TEST_ASSERT_LESS_THAN_MESSAGE(rst, clk, "the clock must be enabled before the reset");
}

void test_init_waits_for_the_engine_after_the_reset(void)
{
    mbist_init(&CONFIG);

    int rst   = _find_command(MBIST_COMMAND_RST_MASK);
    int delay = _find(EV_DELAY, 0);

    TEST_ASSERT_GREATER_OR_EQUAL(0, delay);
    TEST_ASSERT_LESS_THAN_MESSAGE(delay, rst, "the delay must follow the reset");
    // the specification requires at least 5 cycles of the MBIST clock
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(5, _events[delay].a);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, _find(EV_WRITE, (size_t)delay),
                                         "no register access after the delay");
}

void test_init_writes_the_configured_retention_times(void)
{
    mbist_init(&CONFIG);

    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_1, _reg(MBIST_RETENTION_1_ADDR));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_2, _reg(MBIST_RETENTION_2_ADDR));
}

void test_init_packs_both_patterns_into_the_pattern_register(void)
{
    mbist_init(&CONFIG);

    // PATTERN[A] is the low byte, PATTERN[B] the second one
    TEST_ASSERT_EQUAL_HEX32(_CONFIG_PATTERN, _reg(MBIST_PATTERN_ADDR));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.pattern_a,
                            FIELD_GET(MBIST_PATTERN_A_MASK, _reg(MBIST_PATTERN_ADDR)));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.pattern_b,
                            FIELD_GET(MBIST_PATTERN_B_MASK, _reg(MBIST_PATTERN_ADDR)));
}

void test_init_enables_the_done_interrupt(void)
{
    mbist_init(&CONFIG);

    TEST_ASSERT_EQUAL_HEX32(MBIST_INT_EN_DONE_EN_MASK, _reg(MBIST_INT_EN_ADDR));
}

void test_init_leaves_the_engine_disabled(void)
{
    mbist_init(&CONFIG);

    TEST_ASSERT_EQUAL_HEX32(0, _reg(MBIST_CONFIG_ADDR));
}

void test_init_does_not_keep_the_previous_setup(void)
{
    // a setup left in the engine by a previous user
    _set_reg(MBIST_CONFIG_ADDR, MBIST_CONFIG_MODE_MASK
                                | FIELD_PREP(MBIST_CONFIG_TEST_TYPE_MASK, MBIST_TEST_MARCH_C));
    _set_reg(MBIST_RETENTION_1_ADDR, 0xDEAD);
    _set_reg(MBIST_PATTERN_ADDR, 0xBEEF);

    mbist_init(&CONFIG);

    // the reset must write nothing back - the first write after it is already
    // the configuration of this call
    int rst  = _find_command(MBIST_COMMAND_RST_MASK);
    int next = _find(EV_WRITE, (size_t)rst + 1);

    TEST_ASSERT_GREATER_OR_EQUAL(0, next);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(MBIST_RETENTION_1_ADDR, _events[next].a,
                                    "the previous setup must not be restored");
    TEST_ASSERT_EQUAL_HEX32(0, _reg(MBIST_CONFIG_ADDR));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_1, _reg(MBIST_RETENTION_1_ADDR));
    TEST_ASSERT_EQUAL_HEX32(_CONFIG_PATTERN, _reg(MBIST_PATTERN_ADDR));
}

void test_reset_of_a_running_test_never_restores_the_engine_enable(void)
{
    _given_initialized();
    _set_reg(MBIST_CONFIG_ADDR, MBIST_CONFIG_MODE_MASK);
    model.run_finishes = TS_FALSE;      // makes the driver reset the running test

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    // the engine is enabled while the test runs, so the setup restored by the
    // reset must not hand the tested memories over again
    int rst = _find_command(MBIST_COMMAND_RST_MASK);
    int cfg = _find_write(MBIST_CONFIG_ADDR, (size_t)rst + 1);

    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, cfg, "the reset must restore the setup");
    TEST_ASSERT_FALSE_MESSAGE(_events[cfg].b & MBIST_CONFIG_MBIST_EN_MASK,
                              "MBIST_EN must not be restored");
    TEST_ASSERT_TRUE_MESSAGE(_events[cfg].b & MBIST_CONFIG_MODE_MASK,
                             "MODE must be restored");
}

/***************************************************************************************************
*   mbist_exec_test()
***************************************************************************************************/

void test_exec_test_passes_when_every_channel_passes(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_configures_the_requested_test_type(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_C, _CHNLS));

    int cfg = _find_write(MBIST_CONFIG_ADDR, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, cfg);
    TEST_ASSERT_EQUAL_HEX32(MBIST_TEST_MARCH_C,
                            FIELD_GET(MBIST_CONFIG_TEST_TYPE_MASK, _events[cfg].b));
}

void test_exec_test_fits_the_memory_clear_type_into_the_field(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MEM_CLR, _CHNLS));

    // MEM_CLR is 0x4, so the 3 bit wide field is needed
    int cfg = _find_write(MBIST_CONFIG_ADDR, 0);
    TEST_ASSERT_EQUAL_HEX32(MBIST_TEST_MEM_CLR,
                            FIELD_GET(MBIST_CONFIG_TEST_TYPE_MASK, _events[cfg].b));
}

void test_exec_test_keeps_the_mode_of_the_caller(void)
{
    _given_initialized();
    _set_reg(MBIST_CONFIG_ADDR, MBIST_CONFIG_MODE_MASK);

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    TEST_ASSERT_TRUE_MESSAGE(_reg(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MODE_MASK,
                             "MODE must be kept");
}

void test_exec_test_rejects_the_crc_test_without_touching_the_engine(void)
{
    _given_initialized();

    // TSMBIST maintains no result registers for CRC, so it cannot be evaluated
    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_CRC, _CHNLS));

    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, _find(EV_WRITE, 0), "no register may be written");
    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, _find(EV_WAIT, 0), "nothing may be waited for");
}

void test_exec_test_selects_the_channels_before_enabling_the_engine(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    int mem_sel = _find_write(MBIST_MEM_SEL_ADDR, 0);
    int enable  = -1;
    int prepare = _find_command(MBIST_COMMAND_PREPARE_MASK);

    // mbist_en reflects MEM_SEL as soon as the engine is enabled, so the channel
    // selection must be in place first
    for (size_t i = 0; i < _event_count; i++)
    {
        if ((_events[i].kind == EV_WRITE) && (_events[i].a == MBIST_CONFIG_ADDR)
            && (_events[i].b & MBIST_CONFIG_MBIST_EN_MASK))
        {
            enable = (int)i;
            break;
        }
    }
    TEST_ASSERT_GREATER_OR_EQUAL(0, mem_sel);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, enable, "the engine must be enabled");
    TEST_ASSERT_GREATER_OR_EQUAL(0, prepare);
    TEST_ASSERT_LESS_THAN_MESSAGE(enable, mem_sel, "MEM_SEL must precede MBIST_EN");
    TEST_ASSERT_LESS_THAN_MESSAGE(prepare, enable, "MBIST_EN must precede PREPARE");
    TEST_ASSERT_EQUAL_HEX32(_CHNLS, _reg(MBIST_MEM_SEL_ADDR));
}

void test_exec_test_prepares_the_engine_before_starting_the_test(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    int prepare = _find_command(MBIST_COMMAND_PREPARE_MASK);
    int start   = _find_command(MBIST_COMMAND_START_MASK);

    TEST_ASSERT_GREATER_OR_EQUAL(0, prepare);
    TEST_ASSERT_GREATER_OR_EQUAL(0, start);
    TEST_ASSERT_LESS_THAN_MESSAGE(start, prepare, "PREPARE must precede START");
}

void test_exec_test_disables_the_engine_when_the_test_is_over(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    TEST_ASSERT_FALSE_MESSAGE(_reg(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MBIST_EN_MASK,
                              "the engine must not keep the memories taken over");
}

void test_exec_test_fails_when_the_channels_do_not_report_progress(void)
{
    _given_initialized();
    model.prepare_sets_progress = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, _find_command(MBIST_COMMAND_START_MASK),
                                  "the test must not start");
}

void test_exec_test_fails_when_only_a_part_of_the_channels_reports_progress(void)
{
    _given_initialized();
    model.prepare_sets_progress = TS_FALSE;
    // only one channel of the tested set is in progress
    _set_reg(MBIST_TEST_PROGRESS_ADDR, MBIST_CHN_SPECT_DRAM_IN);

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_when_an_error_is_already_reported(void)
{
    _given_initialized();
    model.prepare_forces_error = MBIST_CHN_CPB_CMD_BUF;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, _find_command(MBIST_COMMAND_START_MASK),
                                  "the test must not start");
}

void test_exec_test_fails_when_a_result_is_already_reported(void)
{
    _given_initialized();
    model.prepare_forces_result = MBIST_CHN_CPB_CMD_BUF;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_when_the_engine_still_reports_done(void)
{
    _given_initialized();
    model.prepare_keeps_done = TS_TRUE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_ignores_a_reported_result_of_an_untested_channel(void)
{
    _given_initialized();
    model.prepare_forces_result = _CHNL_OTHER;

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_when_a_channel_reports_an_error(void)
{
    _given_initialized();
    model.error_chnls = MBIST_CHN_CPB_CMD_BUF;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_on_a_reported_error_even_with_the_result_set(void)
{
    _given_initialized();
    // an engine which reports both a result and an error must not pass the test
    model.error_chnls         = MBIST_CHN_CPB_CMD_BUF;
    model.result_despite_error = TS_TRUE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_when_a_channel_stays_in_progress(void)
{
    _given_initialized();
    model.keeps_progress = TS_TRUE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_ignores_the_channels_which_were_not_tested(void)
{
    _given_initialized();
    // results of a previous test left in the registers for another channel
    _set_reg(MBIST_TEST_ERROR_ADDR, _CHNL_OTHER);
    _set_reg(MBIST_TEST_PROGRESS_ADDR, _CHNL_OTHER);

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_waits_for_the_worst_case_test_duration(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    int wait = _find(EV_WAIT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, wait);
    // the retention times of the configuration are a part of the MARCH tests
    u32 expected = _EXPECTED_BASE_US
                 + (CONFIG.retention_1 / HW_CLOCK_MHZ) + (CONFIG.retention_2 / HW_CLOCK_MHZ);
    TEST_ASSERT_EQUAL_UINT32(expected, _events[wait].a);
}

void test_exec_test_fails_when_the_engine_does_not_finish(void)
{
    _given_initialized();
    model.run_finishes = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_fails_when_the_engine_only_pauses(void)
{
    _given_initialized();
    // STATUS[PAUSED] does not release the wait, only STATUS[DONE] does
    model.signals_paused = TS_TRUE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));
}

void test_exec_test_resets_an_unfinished_test_before_releasing_the_engine(void)
{
    _given_initialized();
    model.run_finishes = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    int start = _find_command(MBIST_COMMAND_START_MASK);
    int rst   = _find_command_from(MBIST_COMMAND_RST_MASK, (size_t)start);

    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, rst, "the running test must be stopped");
    TEST_ASSERT_FALSE(_reg(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MBIST_EN_MASK);
}

void test_exec_test_keeps_the_setup_when_the_engine_does_not_finish(void)
{
    _given_initialized();
    model.run_finishes = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_exec_test(MBIST_TEST_MARCH_SS, _CHNLS));

    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_1, _reg(MBIST_RETENTION_1_ADDR));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_2, _reg(MBIST_RETENTION_2_ADDR));
    TEST_ASSERT_EQUAL_HEX32(_CONFIG_PATTERN, _reg(MBIST_PATTERN_ADDR));
    TEST_ASSERT_EQUAL_HEX32(MBIST_INT_EN_DONE_EN_MASK, _reg(MBIST_INT_EN_ADDR));
}

/***************************************************************************************************
*   mbist_erase()
***************************************************************************************************/

void test_erase_succeeds_on_a_healthy_engine(void)
{
    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));
}

void test_erase_runs_the_memory_clear_pattern_on_the_selected_channels(void)
{
    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    int prepare = _find_command(MBIST_COMMAND_PREPARE_MASK);
    int start   = _find_command(MBIST_COMMAND_START_MASK);

    TEST_ASSERT_GREATER_OR_EQUAL(0, prepare);
    TEST_ASSERT_GREATER_OR_EQUAL(0, start);
    TEST_ASSERT_EQUAL_HEX32(_CHNLS, _reg(MBIST_MEM_SEL_ADDR));

    // the memory clear type has to be configured before the engine is prepared
    int cfg = -1;
    for (size_t i = 0; i < (size_t)prepare; i++)
    {
        if ((_events[i].kind == EV_WRITE) && (_events[i].a == MBIST_CONFIG_ADDR)
            && (FIELD_GET(MBIST_CONFIG_TEST_TYPE_MASK, _events[i].b) == MBIST_TEST_MEM_CLR))
        {
            cfg = (int)i;
        }
    }
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, cfg, "MEM_CLR must be configured");
}

void test_erase_wakes_the_engine_up_and_suspends_it_again(void)
{
    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    int clk_en  = _find(EV_CLK_EN, 0);
    int clk_dis = _find(EV_CLK_DIS, 0);

    TEST_ASSERT_GREATER_OR_EQUAL(0, clk_en);
    TEST_ASSERT_GREATER_OR_EQUAL(0, clk_dis);
    TEST_ASSERT_LESS_THAN_MESSAGE(clk_dis, clk_en, "the clock must be enabled first");
    TEST_ASSERT_LESS_THAN_MESSAGE(clk_dis, _find_command(MBIST_COMMAND_START_MASK),
                                  "the test must run before the clock is disabled");
}

void test_erase_enables_the_done_interrupt_without_clearing_the_others(void)
{
    _given_initialized();
    _set_reg(MBIST_INT_EN_ADDR, MBIST_INT_EN_PAUSED_EN_MASK);

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    TEST_ASSERT_EQUAL_HEX32(MBIST_INT_EN_PAUSED_EN_MASK | MBIST_INT_EN_DONE_EN_MASK,
                            _reg(MBIST_INT_EN_ADDR));
}

void test_erase_keeps_the_setup_of_the_caller(void)
{
    _given_initialized();

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    // the reset in the erase must not throw the configuration away
    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_1, _reg(MBIST_RETENTION_1_ADDR));
    TEST_ASSERT_EQUAL_HEX32(CONFIG.retention_2, _reg(MBIST_RETENTION_2_ADDR));
    TEST_ASSERT_EQUAL_HEX32(_CONFIG_PATTERN, _reg(MBIST_PATTERN_ADDR));
}

void test_erase_restores_the_config_register_of_the_caller(void)
{
    _given_initialized();
    u32 cfg = MBIST_CONFIG_MODE_MASK
            | FIELD_PREP(MBIST_CONFIG_TEST_TYPE_MASK, MBIST_TEST_MARCH_SS);
    _set_reg(MBIST_CONFIG_ADDR, cfg);

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    TEST_ASSERT_EQUAL_HEX32(cfg, _reg(MBIST_CONFIG_ADDR));
}

void test_erase_does_not_restore_the_engine_enable_of_the_caller(void)
{
    _given_initialized();
    _set_reg(MBIST_CONFIG_ADDR, MBIST_CONFIG_MBIST_EN_MASK);

    TEST_ASSERT_EQUAL_HEX8(TS_TRUE, mbist_erase(_CHNLS));

    TEST_ASSERT_FALSE_MESSAGE(_reg(MBIST_CONFIG_ADDR) & MBIST_CONFIG_MBIST_EN_MASK,
                              "the engine must not be left enabled");
}

void test_erase_fails_and_does_not_clear_when_the_preparation_fails(void)
{
    model.prepare_sets_progress = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_erase(_CHNLS));

    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, _find_command(MBIST_COMMAND_START_MASK),
                                  "the memory clear must not start");
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, _find(EV_CLK_DIS, 0),
                                         "the clock must be disabled anyway");
}

void test_erase_fails_when_the_engine_does_not_finish(void)
{
    model.run_finishes = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_erase(_CHNLS));
}

void test_erase_resets_an_unfinished_clear_before_disabling_the_clock(void)
{
    model.run_finishes = TS_FALSE;

    TEST_ASSERT_EQUAL_HEX8(TS_FALSE, mbist_erase(_CHNLS));

    int start   = _find_command(MBIST_COMMAND_START_MASK);
    int rst     = _find_command_from(MBIST_COMMAND_RST_MASK, (size_t)start);
    int clk_dis = _find(EV_CLK_DIS, 0);

    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, rst, "the running test must be stopped");
    TEST_ASSERT_LESS_THAN_MESSAGE(clk_dis, rst, "the reset needs the clock still running");
}

/***************************************************************************************************
*   irq_mbist_handler()
***************************************************************************************************/

void test_isr_clears_the_done_status(void)
{
    _set_reg(MBIST_STATUS_ADDR, MBIST_STATUS_DONE_MASK);

    irq_mbist_handler();

    TEST_ASSERT_FALSE_MESSAGE(_reg(MBIST_STATUS_ADDR) & MBIST_STATUS_DONE_MASK,
                              "STATUS[DONE] must be acknowledged");
}

void test_isr_acknowledges_the_paused_status_too(void)
{
    _set_reg(MBIST_STATUS_ADDR, MBIST_STATUS_PAUSED_MASK);

    irq_mbist_handler();

    TEST_ASSERT_EQUAL_HEX32(0, _reg(MBIST_STATUS_ADDR));
}

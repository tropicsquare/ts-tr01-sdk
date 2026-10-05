/**
 * @file test_prng.c
 * @brief Unit tests for prng_seed() and prng_read() — drv/prng.c
 *
 * prng_read() reports whether it has a sequence to continue, so that the caller
 * can decide what an unseeded PRNG means. The tests cover:
 *   - the unseeded state: TS_FALSE and *value left alone,
 *   - a NULL output pointer: TS_FALSE and no dereference,
 *   - the seeded state: TS_TRUE and the LCG advanced before the value is taken,
 *   - reproducibility: the same seed replays the same sequence.
 *
 * @note prng.c keeps the seed in a file-static and offers no way back to the
 *       unseeded state, so the unseeded test must run first. Unity executes the
 *       tests in the order they are defined here.
 */

#include "unity.h"
#include "prng.h"

/* The LCG constants of prng.c, to predict the sequence from a known seed. */
#define LCG_MULTIPLIER 1664525U
#define LCG_INCREMENT  1013904223U

#define SEED_A 0x12345678U
#define SEED_B 0xFEEDBEEFU

void setUp(void)    {}
void tearDown(void) {}

/* --- unseeded PRNG (must stay the first test, see the file comment) -------- */

void test_unseeded_reports_failure_and_keeps_value(void)
{
    u32 value = 0xDEADBEEFU;

    TEST_ASSERT_EQUAL(TS_FALSE, prng_read(&value));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFU, value); // untouched, nothing was delivered
}

/* --- NULL output pointer -------------------------------------------------- */

void test_null_value_reports_failure(void)
{
    /* If NULL was dereferenced, the test would crash. */
    TEST_ASSERT_EQUAL(TS_FALSE, prng_read(NULL));

    prng_seed(SEED_A);
    TEST_ASSERT_EQUAL(TS_FALSE, prng_read(NULL));
}

/* --- seeded PRNG ---------------------------------------------------------- */

void test_seeded_returns_advanced_seed(void)
{
    u32 value = 0;

    prng_seed(SEED_A);

    /* The seed itself must never be handed out - the LCG advances first. */
    TEST_ASSERT_EQUAL(TS_TRUE, prng_read(&value));
    TEST_ASSERT_EQUAL_HEX32((SEED_A * LCG_MULTIPLIER) + LCG_INCREMENT, value);
    TEST_ASSERT_NOT_EQUAL_HEX32(SEED_A, value);
}

void test_consecutive_reads_follow_the_lcg(void)
{
    u32 expected = SEED_B;
    u32 value    = 0;

    prng_seed(SEED_B);

    for (int i = 0; i < 8; i++)
    {
        expected = (expected * LCG_MULTIPLIER) + LCG_INCREMENT;

        TEST_ASSERT_EQUAL(TS_TRUE, prng_read(&value));
        TEST_ASSERT_EQUAL_HEX32(expected, value);
    }
}

void test_same_seed_replays_the_same_sequence(void)
{
    u32 first[4]  = {0};
    u32 second[4] = {0};

    prng_seed(SEED_A);
    for (size_t i = 0; i < 4; i++)
    {
        TEST_ASSERT_EQUAL(TS_TRUE, prng_read(&first[i]));
    }

    prng_seed(SEED_A);
    for (size_t i = 0; i < 4; i++)
    {
        TEST_ASSERT_EQUAL(TS_TRUE, prng_read(&second[i]));
    }

    TEST_ASSERT_EQUAL_HEX32_ARRAY(first, second, 4);
}

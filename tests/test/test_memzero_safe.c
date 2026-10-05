/**
 * @file test_memzero_safe.c
 * @brief Unit tests for memzero_safe() — common/util.c
 *
 * memzero_safe() overwrites a memory region with zeros without touching the PRNG,
 * so it is usable before prng_seed() and wherever the zero value itself matters.
 * The tests verify that exactly 'size' bytes starting at 'dest' become zero — no
 * under- or over-run (checked with guard/sentinel bytes around the region) — for
 * every start offset and size combination, and that the PRNG is never called
 * (mock_prng would fail the test if it were).
 */

#include "unity.h"
#include "util.h"
#include "mock_prng.h"

void setUp(void)    {}
void tearDown(void) {}

/* Sentinel written around the target region to catch any out-of-bounds write. */
#define GUARD 0xA5U

/*
 * Zero a region at the given offset and size inside a guarded buffer, then verify
 * the region and both guard areas byte by byte.
 */
static void zero_and_verify(size_t off, size_t size)
{
    u8 buf[64];

    TEST_ASSERT_TRUE(off + size <= sizeof(buf));

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = GUARD;
    }

    memzero_safe(&buf[off], size);

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        u8 expected = ((i >= off) && (i < (off + size))) ? 0x00U : GUARD;
        TEST_ASSERT_EQUAL_HEX8(expected, buf[i]);
    }
}

/* --- size 0 (no-op) ------------------------------------------------------- */

void test_size_0_does_nothing(void) { zero_and_verify(8, 0); }

/* Test that NULL pointer is never dereferenced if size is 0.
 * If NULL was dereferenced, test would crash.
 */
void test_null_dest_zero_size_is_safe_noop(void)
{
    memzero_safe(NULL, 0);
    TEST_PASS_MESSAGE("survived NULL/0 with no dereference");
}

/* --- single bytes and whole words ----------------------------------------- */

void test_size_1(void)                   { zero_and_verify(8, 1);  }
void test_size_4(void)                   { zero_and_verify(8, 4);  }
void test_size_32(void)                  { zero_and_verify(8, 32); }

/* --- unaligned start ------------------------------------------------------ */

void test_unaligned_start(void)          { zero_and_verify(11, 16); }

/* --- full sweep: every start alignment against every size ----------------- */

void test_all_offsets_and_sizes(void)
{
    for (size_t off = 8; off < 12; off++)
    {
        for (size_t size = 0; size <= 32; size++)
        {
            zero_and_verify(off, size);
        }
    }
}

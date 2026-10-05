/**
 * @file test_memerase_safe.c
 * @brief Unit tests for memerase_safe() — common/util.c
 *
 * memerase_safe() overwrites a memory region with pseudo-random bytes pulled
 * from the PRNG (drv/prng.c). By mocking the PRNG, every "random" word becomes
 * deterministic, which lets us verify the behaviour exactly:
 *   - the number of PRNG refills: one 32-bit word per sizeof(u32) bytes,
 *   - the little-endian order in which each word's bytes are written,
 *   - that exactly 'size' bytes are touched — no under- or over-run
 *     (checked with guard/sentinel bytes surrounding the target region),
 *   - the fallback to zeros when prng_read() reports an unseeded PRNG.
 *
 * The PRNG is mocked with a stub instead of Expect calls, because prng_read()
 * delivers its value through an output parameter.
 */

#include "unity.h"
#include "util.h"
#include "mock_prng.h"

/* Number of prng_read() calls the stub has served. */
static size_t prng_calls;

/* What the stub reports: TS_TRUE dispenses WORDS[], TS_FALSE emulates a PRNG
 * which has not been seeded yet. */
static ts_bool prng_result;

static ts_bool prng_read_stub(u32 *value, int cmock_num_calls);

void setUp(void)
{
    prng_calls  = 0;
    prng_result = TS_TRUE;
    prng_read_Stub(prng_read_stub);
}

void tearDown(void) {}

/* Sentinel written around the target region to catch any out-of-bounds write. */
#define GUARD 0xA5U

/* Deterministic "random" words the mocked PRNG hands back, one per 4 bytes.
 * Bytes within each word are distinct so byte-ordering bugs are visible.
 * Enough entries for the largest region tested (32 bytes -> 8 words). */
static const u32 WORDS[] = {
    0x11223344U, 0x55667788U, 0x99AABBCCU, 0xDDEEFF00U,
    0x0A1B2C3DU, 0x4E5F6071U, 0x8293A4B5U, 0xC6D7E8F9U,
};
#define WORDS_COUNT (sizeof(WORDS) / sizeof(WORDS[0]))

/*
 * Stands in for prng_read(): dispenses WORDS[] in order, one word per call. On a
 * simulated failure it leaves *value alone, exactly like the real one - so the zeros
 * the unseeded test expects have to come from memerase_safe() itself.
 */
static ts_bool prng_read_stub(u32 *value, int cmock_num_calls)
{
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_TRUE((size_t)cmock_num_calls < WORDS_COUNT);

    prng_calls++;

    if (prng_result == TS_TRUE)
    {
        *value = WORDS[cmock_num_calls];
    }
    return prng_result;
}

/*
 * Erase a region of the given size and verify the result exactly. The region
 * is placed inside a larger guarded buffer so we can also assert that nothing
 * outside [off, off+size) was modified.
 */
static void erase_and_verify(size_t size)
{
    const size_t off     = 8;                                    /* leading guard bytes */
    const size_t refills  = (size + sizeof(u32) - 1U) / sizeof(u32); /* one word per 4 bytes */
    u8 buf[64];

    /* Sanity-guard the test fixture itself. */
    TEST_ASSERT_TRUE(refills <= WORDS_COUNT);
    TEST_ASSERT_TRUE(off + size <= sizeof(buf));

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = GUARD;
    }

    memerase_safe(&buf[off], size);

    /* The PRNG must be asked for exactly one word per sizeof(u32) bytes.
     * (For size 0 that means it must not be touched at all.) */
    TEST_ASSERT_EQUAL_size_t(refills, prng_calls);

    /* Guard bytes before and after the region must be untouched. */
    for (size_t i = 0; i < off; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(GUARD, buf[i]);
    }
    for (size_t i = off + size; i < sizeof(buf); i++)
    {
        TEST_ASSERT_EQUAL_HEX8(GUARD, buf[i]);
    }

    /* Erased bytes must equal the little-endian bytes of the PRNG words. */
    for (size_t i = 0; i < size; i++)
    {
        // get a word and from the word a byte that is expected to be in buf
        u8 expected = (u8)((WORDS[i / sizeof(u32)] >> (8U * (i % sizeof(u32)))) & 0xFFU);
        TEST_ASSERT_EQUAL_HEX8(expected, buf[off + i]);
    }
}

/* --- size 0 (no-op): PRNG must not be called, nothing modified ------------- */

void test_size_0_does_nothing(void)      { erase_and_verify(0);  }

/* Test that NULL pointer is never dereferenced if size is 0.
 * If NULL was dereferenced, test would crash.
 */
void test_null_dest_zero_size_is_safe_noop(void)
{
    memerase_safe(NULL, 0);
    TEST_ASSERT_EQUAL_size_t(0, prng_calls);
    TEST_PASS_MESSAGE("survived NULL/0 with no dereference and no PRNG call");
}

/* --- sizes 1..4: partial first word up to one full word ------------------- */

void test_size_1(void)                   { erase_and_verify(1);  }
void test_size_2(void)                   { erase_and_verify(2);  }
void test_size_3(void)                   { erase_and_verify(3);  }
void test_size_4_one_full_word(void)     { erase_and_verify(4);  }

/* --- exact multiples of sizeof(u32) --------------------------------------- */

void test_size_8_two_words(void)         { erase_and_verify(8);  }
void test_size_16_four_words(void)       { erase_and_verify(16); }
void test_size_32_eight_words(void)      { erase_and_verify(32); }

/* --- sizes not aligned to sizeof(u32) (partial trailing word) ------------- */

void test_size_5_unaligned(void)         { erase_and_verify(5);  }
void test_size_7_unaligned(void)         { erase_and_verify(7);  }
void test_size_13_unaligned(void)        { erase_and_verify(13); }
void test_size_23_unaligned(void)        { erase_and_verify(23); }

/* --- unseeded PRNG: the region is still erased, just with zeros ------------ */

void test_unseeded_prng_erases_with_zeros(void)
{
    const size_t off  = 8;
    const size_t size = 13;
    u8 buf[64];

    prng_result = TS_FALSE; // emulate prng_read() called before prng_seed()

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = GUARD;
    }

    memerase_safe(&buf[off], size);

    /* The PRNG is still asked once per word, it just does not deliver a value. */
    TEST_ASSERT_EQUAL_size_t((size + sizeof(u32) - 1U) / sizeof(u32), prng_calls);

    for (size_t i = 0; i < off; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(GUARD, buf[i]);
    }
    for (size_t i = off; i < off + size; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(0x00, buf[i]);
    }
    for (size_t i = off + size; i < sizeof(buf); i++)
    {
        TEST_ASSERT_EQUAL_HEX8(GUARD, buf[i]);
    }
}

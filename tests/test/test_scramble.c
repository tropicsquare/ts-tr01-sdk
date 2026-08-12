/**
 * @file test_scramble.c
 * @brief Unit tests for the scrambling utilities — drv/scramble.c
 *
 * Regression guards for scrambling bugs found in early 2026:
 *   - 1b61e88: LSB/MSB packing order was inverted in scramble_value()
 *   - 790e924: OTP needed scramble_value_reversed() for ACAB compat
 *   - 40e2d34: Flash PAGE init must use full word size (8), shuffle only 7
 *
 * API under test
 * --------------
 *   scramble_init()             — identity permutation {0,1,2,...}
 *   scramble_shuffle()          — Fisher-Yates shuffle driven by PUF seed
 *   scramble_value()            — LSB-first: seq[0] → bits 3:0
 *   scramble_value_reversed()   — MSB-first: seq[0] → MSB nibble (ACAB compat)
 */

#include "unity.h"
#include "scramble.h"
#include <string.h>

#define WORD_NIBBLES  (32u / 4u)   /* 8 — max nibbles per u32 */

void setUp(void)    {}
void tearDown(void) {}

/* === scramble_init() ====================================================== */

void test_init_identity(void)
{
    u8 seq[8];
    scramble_init(seq, sizeof(seq));
    for (u8 i = 0; i < sizeof(seq); i++)
        TEST_ASSERT_EQUAL_UINT8(i, seq[i]);
}

void test_init_single_element(void)
{
    u8 seq[1] = {0xFF};
    scramble_init(seq, 1);
    TEST_ASSERT_EQUAL_UINT8(0, seq[0]);
}

/* === scramble_value() — LSB-first (regression: 1b61e88) =================== */

void test_value_lsb_first_packing(void)
{
    /* seq[0]→bits 3:0, seq[3]→bits 15:12 → {0,1,2,3} → 0x3210 */
    u8 seq[4] = {0, 1, 2, 3};
    TEST_ASSERT_EQUAL_UINT32(0x00003210u, scramble_value(seq, 4));
}

void test_value_full_word(void)
{
    /* Full 8-nibble identity → 0x76543210 */
    u8 seq[WORD_NIBBLES];
    scramble_init(seq, WORD_NIBBLES);
    TEST_ASSERT_EQUAL_UINT32(0x76543210u, scramble_value(seq, WORD_NIBBLES));
}

void test_value_masks_to_4_bits(void)
{
    /* Only low 4 bits used: 0xFF → 0x0F */
    u8 seq[1] = {0xFF};
    TEST_ASSERT_EQUAL_UINT32(0x0000000Fu, scramble_value(seq, 1));
}

void test_value_each_nibble_position(void)
{
    /* Walk every (slot, nibble) pair: marker at seq[pos] must land in
     * bits [4*pos+3 : 4*pos] and leave all other bits zero.
     * Exhaustive per-slot check — hardens against the kind of per-position
     * packing regression that 1b61e88 was. */
    for (u8 pos = 0; pos < WORD_NIBBLES; pos++) {
        for (u8 v = 0; v <= 0xF; v++) {
            u8 seq[WORD_NIBBLES] = {0};
            seq[pos] = v;
            u32 expected = (u32)v << (pos * 4);
            TEST_ASSERT_EQUAL_HEX32(expected, scramble_value(seq, WORD_NIBBLES));
        }
    }
}

/* === scramble_value_reversed() — MSB-first (regression: 790e924) ========== */

void test_reversed_msb_first_packing(void)
{
    /* seq[0]→MSB nibble, seq[3]→bits 3:0 → {0,1,2,3} → 0x0123 */
    u8 seq[4] = {0, 1, 2, 3};
    TEST_ASSERT_EQUAL_UINT32(0x00000123u, scramble_value_reversed(seq, 4));
}

void test_reversed_full_word(void)
{
    /* Full 8-nibble identity → 0x01234567
     * Pins the MSB-first packing for OTP (regression: 790e924). */
    u8 seq[WORD_NIBBLES];
    scramble_init(seq, WORD_NIBBLES);
    TEST_ASSERT_EQUAL_UINT32(0x01234567u, scramble_value_reversed(seq, WORD_NIBBLES));
}

void test_value_reversed_each_nibble_position(void)
{
    /* MSB-first counterpart of test_value_each_nibble_position:
     * seq[pos] lands at slot (n-1-pos), so the marker shifts by
     * (WORD_NIBBLES-1-pos)*4. Walk every slot × every nibble. */
    for (u8 pos = 0; pos < WORD_NIBBLES; pos++) {
        for (u8 v = 0; v <= 0xF; v++) {
            u8 seq[WORD_NIBBLES] = {0};
            seq[pos] = v;
            u32 expected = (u32)v << ((WORD_NIBBLES - 1 - pos) * 4);
            TEST_ASSERT_EQUAL_HEX32(expected, scramble_value_reversed(seq, WORD_NIBBLES));
        }
    }
}

/* === scramble_shuffle() =================================================== */

void test_shuffle_deterministic(void)
{
    u8 seq1[8], seq2[8];
    const u8 seed[8] = {42, 17, 255, 0, 1, 88, 33, 7};

    scramble_init(seq1, 8);
    scramble_init(seq2, 8);
    scramble_shuffle(seq1, 8, seed);
    scramble_shuffle(seq2, 8, seed);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(seq1, seq2, 8);
}

void test_shuffle_produces_permutation(void)
{
    u8 seq[8];
    const u8 seed[8] = {3, 1, 4, 1, 5, 9, 2, 6};
    u8 seen[8] = {0};

    scramble_init(seq, 8);
    scramble_shuffle(seq, 8, seed);

    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_TRUE_MESSAGE(seq[i] < 8, "out of range");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, seen[seq[i]], "duplicate");
        seen[seq[i]] = 1;
    }
}

void test_shuffle_different_seeds_differ(void)
{
    u8 seq1[8], seq2[8];
    const u8 seed1[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    const u8 seed2[8] = {7, 6, 5, 4, 3, 2, 1, 0};

    scramble_init(seq1, 8);
    scramble_init(seq2, 8);
    scramble_shuffle(seq1, 8, seed1);
    scramble_shuffle(seq2, 8, seed2);

    TEST_ASSERT_NOT_EQUAL(scramble_value(seq1, 8), scramble_value(seq2, 8));
}

/* === Flash PAGE: init(8) + shuffle(7) + value(8) — regression: 40e2d34 ==== */

void test_flash_page_seq7_fixed_at_identity(void)
{
    /* flash.c: scramble_init(seq, 8); scramble_shuffle(seq, 7, seed);
     * seq[7] stays at 7 (never shuffled).
     * LSB-first: seq[7] → MSB nibble (bits 31:28). */
    u8  seq1[8], seq2[8];
    const u8 seed_a[7] = {0};
    const u8 seed_b[7] = {0, 0, 0, 0, 0, 0, 3};

    scramble_init(seq1, 8);
    scramble_shuffle(seq1, 7, seed_a);
    u32 val_a = scramble_value(seq1, 8);

    scramble_init(seq2, 8);
    scramble_shuffle(seq2, 7, seed_b);
    u32 val_b = scramble_value(seq2, 8);

    /* seq[7]=7 always lands in MSB nibble */
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(7u, (val_a >> 28) & 0xFu,
        "seq[7] must stay at identity 7");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(7u, (val_b >> 28) & 0xFu,
        "seq[7] must stay at identity 7");

    /* Shuffled part differs between seeds */
    TEST_ASSERT_NOT_EQUAL_MESSAGE(val_a & 0x0FFFFFFFu, val_b & 0x0FFFFFFFu,
        "shuffled nibbles must differ between seeds");

    /* All shuffled values stay within {0..6} */
    for (int i = 0; i < 7; i++)
        TEST_ASSERT_TRUE_MESSAGE(seq1[i] <= 6, "PAGE value out of range");
}

/* === OTP: init(11) + shuffle(11) + reversed(8) + reversed(3) ============== */

void test_otp_uses_reversed_split(void)
{
    /* otp.c: scramble_value_reversed() for both halves (ACAB compat).
     * All 11 shuffled, split at 8. */
    u8  seq1[11], seq2[11];
    const u8 seed_a[11] = {0};
    const u8 seed_b[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};

    scramble_init(seq1, 11);
    scramble_shuffle(seq1, 11, seed_a);
    u32 reg0_a = scramble_value_reversed(seq1, 8);
    u32 reg1_a = scramble_value_reversed(seq1 + 8, 3);

    scramble_init(seq2, 11);
    scramble_shuffle(seq2, 11, seed_b);
    u32 reg0_b = scramble_value_reversed(seq2, 8);
    u32 reg1_b = scramble_value_reversed(seq2 + 8, 3);

    TEST_ASSERT_NOT_EQUAL_MESSAGE(reg0_a, reg0_b,
        "OTP SCRAM_0: seeds must produce different values");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(reg1_a, reg1_b,
        "OTP SCRAM_1: seeds must produce different values");
}

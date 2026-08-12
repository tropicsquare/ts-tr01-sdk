/**
 * @file test_ct_memcmp.c
 * @brief Unit tests for ct_memcmp() — common/util.c
 *
 * ct_memcmp() is a constant-time memory comparison used to compare
 * cryptographic material (keys, MACs). A timing side-channel here could leak
 * secret data, so correctness across all byte positions is critical.
 */

#include "unity.h"
#include "util.h"

void setUp(void)    {}
void tearDown(void) {}

/* --- Equal buffers -------------------------------------------------------- */

void test_equal_buffers_returns_zero(void)
{
    u8 a[] = {0x01, 0x02, 0x03, 0x04};
    u8 b[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL_INT(0, ct_memcmp(a, b, sizeof(a)));
}

/* --- Different buffers (position coverage for constant-time property) ----- */

void test_first_byte_differs(void)
{
    u8 a[] = {0xFF, 0x02, 0x03};
    u8 b[] = {0x00, 0x02, 0x03};
    TEST_ASSERT_NOT_EQUAL(0, ct_memcmp(a, b, sizeof(a)));
}

void test_last_byte_differs(void)
{
    /* Would pass incorrectly if implementation short-circuits on early match */
    u8 a[] = {0x01, 0x02, 0xFF};
    u8 b[] = {0x01, 0x02, 0x00};
    TEST_ASSERT_NOT_EQUAL(0, ct_memcmp(a, b, sizeof(a)));
}

void test_middle_byte_differs(void)
{
    u8 a[] = {0xAA, 0xBB, 0xCC};
    u8 b[] = {0xAA, 0x00, 0xCC};
    TEST_ASSERT_NOT_EQUAL(0, ct_memcmp(a, b, sizeof(a)));
}

/* --- Edge cases ----------------------------------------------------------- */

void test_zero_size_returns_zero(void)
{
    u8 a[] = {0xDE, 0xAD};
    u8 b[] = {0xBE, 0xEF};
    TEST_ASSERT_EQUAL_INT(0, ct_memcmp(a, b, 0));
}

void test_same_pointer_returns_zero(void)
{
    u8 buf[] = {0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_INT(0, ct_memcmp(buf, buf, sizeof(buf)));
}

/**
 * @file test_crc16.c
 * @brief Unit tests for crc16() and crc16_byte() — hal/crc16.c
 *
 * CRC-16/BUYPASS: poly 0x8005, init 0x0000, no reflection, no final XOR.
 * Reference check value for "123456789": 0xFEE8
 */

#include "unity.h"
#include "crc16.h"

void setUp(void)    {}
void tearDown(void) {}

/* --- crc16_byte() --------------------------------------------------------- */

void test_byte_zero_data_returns_zero(void)
{
    TEST_ASSERT_EQUAL_UINT16(0x0000, crc16_byte(0x00, 0x0000));
}

void test_byte_0x01_from_zero_returns_0x8005(void)
{
    /* Hand-verified: 0x0100 → 7 shifts → 0x8000 → XOR poly → 0x8005 */
    TEST_ASSERT_EQUAL_UINT16(0x8005, crc16_byte(0x01, 0x0000));
}

/* --- crc16() -------------------------------------------------------------- */

void test_empty_buffer_returns_initial_value(void)
{
    u8 data[] = {0xAB};
    TEST_ASSERT_EQUAL_UINT16(CRC16_INITIAL_VAL, crc16(data, 0));
}

void test_single_byte_matches_crc16_byte(void)
{
    u8 data[] = {0x01};
    TEST_ASSERT_EQUAL_UINT16(crc16_byte(0x01, CRC16_INITIAL_VAL), crc16(data, 1));
}

void test_bulk_equals_incremental(void)
{
    u8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    u16 bulk        = crc16(data, sizeof(data));
    u16 incremental = CRC16_INITIAL_VAL;
    for (size_t i = 0; i < sizeof(data); i++)
        incremental = crc16_byte(data[i], incremental);

    TEST_ASSERT_EQUAL_UINT16(bulk, incremental);
}

void test_known_vector_123456789(void)
{
    const u8 data[] = "123456789";
    TEST_ASSERT_EQUAL_UINT16(0xFEE8, crc16(data, 9));
}

/**
 * @file ftc.c
 * @author Tropic Square
 * @brief FTC (Firmware Testbench Channel) driver source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <stdarg.h>
#include "ftc.h"

#if TS_SIMULATION_BUILD == 1

#define _DRAM_BADDR             (0x00200000)
//Random number location in DRAM - address 0x300 (offset 0xC0)
#define _RNM_DRAM_OFFSET        (0x000000C0)

#define _FTC_REG_READ(offset)           IO_READ_32(FTC_BASE_ADDRESS+(offset))
#define _FTC_REG_WRITE(offset,value)    IO_WRITE_32(FTC_BASE_ADDRESS+(offset), value)

void ftc_test_passed(void)
{
    PTR32_T(FTC_BASE_ADDRESS + FTC_STATUS_ADDR) = FTC_TEST_PASSED_VALUE;
}


void ftc_test_failed(void)
{
    PTR32_T(FTC_BASE_ADDRESS + FTC_STATUS_ADDR) = FTC_TEST_FAILED_VALUE;
}


void ftc_send_to_tb(size_t index, u32 value)
{
    PTR32_T(OFFSET_IO(FTC_BASE_ADDRESS + FTC_FW_TO_TB_0_ADDR, index)) = value;
}


u32 ftc_receive_from_tb(size_t index)
{
    return PTR32_T(OFFSET_IO(FTC_BASE_ADDRESS + FTC_TB_TO_FW_0_ADDR, index));
}


void ftc_print_msg(const char *fmt, ...)
{
    va_list vlist;

    _FTC_REG_WRITE(FTC_MSG_BUF_CTRL_ADDR, FTC_MSG_FLUSH_VALUE);

    va_start(vlist, fmt);
    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;

            while ((*fmt >= '0') && (*fmt <= '9'))
                fmt++; // skip unsupported width specifiers

            if (*fmt == 'l')
                fmt++;

            switch (*fmt)
            {
                case 'c':
                    _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, va_arg(vlist, int));
                    break;

                case 'd': // "%d" does not support negative values
                case 'u':
                    {
                        u32 val = va_arg(vlist, int);
                        char tmp[10];
                        int l=0;

                        do
                        {
                            tmp[l++] = '0' + (val % 10);
                            val /= 10;
                        } while (val);

                        while (l--)
                        {
                            _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, tmp[l]);
                        }
                    }
                    break;

                case 's':
                    {
                        char *s = va_arg(vlist, char *);
                        while (*s)
                        {
                            _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, *s);
                            s++;
                        }
                    }
                    break;

                case 'x':
                    {
                        u32 val = va_arg(vlist, int);
                        u32 mask = 0xF0000000;
                        int i;
                        
                        // Printing '0x' prefix.
                        _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, '0');
                        _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, 'x');

                        for (i=0; i<7; i++)
                        {
                            if (val & mask)
                                break;
                            val <<= 4;
                        }
                        for (; i<8; i++)
                        {
                            int n = (val >> 28) & 0xF;
                            if (n > 9)
                            {
                                _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, 'a' + n - 10);
                            }
                            else
                            {
                                _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, '0' + n);
                            }
                            val <<= 4;
                        }
                    }
                    break;

                default:
                    _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, *fmt);
                    break;
            }
        }
        else
        {
            _FTC_REG_WRITE(FTC_MSG_BUF_IN_ADDR, *fmt);
        }
        fmt++;
    }
    va_end(vlist);

    _FTC_REG_WRITE(FTC_MSG_BUF_CTRL_ADDR, FTC_MSG_PRINT_VALUE);
}

// Random number function
u32 ftc_get_rnd_num (void)
{
    u32 read_data;
    //Random number will be loaded to RAM choosing address 0x300 (offset 0xC0) in dram
    PTR32_T(FTC_BASE_ADDRESS + FTC_RNM_DRAM_OFFSET_ADDR) = _RNM_DRAM_OFFSET;
    //Requesting random number
    PTR32_T(FTC_BASE_ADDRESS + FTC_RNM_CTRL_ADDR) = FTC_RNM_CTRL_RNM_GET_MASK;
    //Fetch random data from memory.
    read_data = PTR32_T(_DRAM_BADDR + _RNM_DRAM_OFFSET*4);

    return read_data;
}
#else // TS_SIMULATION_BUILD

// avoid warning: "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif // ! TS_SIMULATION_BUILD


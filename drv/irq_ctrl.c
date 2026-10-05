/**
 * @file irq_ctrl.c
 * @author Tropic Square
 * @brief Interrupt controller HW driver source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "irq_ctrl.h"
#include "ftc.h"
#include "irq_ctrl_regs.h"
#include "io_ops.h"
#include "tassic_defs.h"
#include "os.h"
#include "cpu.h"
#include "csr.h"

#include "log.h"

LOG_DEF("IRQ");

#define _IRQ_ERR_DEFAULT   1
#define _IRQ_ERR_NM        2
#define _IRQ_ERR_EXCEPTION 0x10000

enum {
    MAP_IRQ_0  = 1,
    MAP_IRQ_1  = 2,
    MAP_IRQ_2  = 3,
    MAP_IRQ_3  = 4,
    MAP_IRQ_4  = 5,
    MAP_IRQ_5  = 6,
    MAP_IRQ_6  = 7,
    MAP_IRQ_7  = 8,
    MAP_IRQ_8  = 9,
    MAP_IRQ_9  = 10,
    MAP_IRQ_10 = 11,
    MAP_IRQ_11 = 12,
    MAP_IRQ_12 = 13,
    MAP_IRQ_13 = 14,
    MAP_IRQ_14 = 15,
    MAP_IRQ_15 = 16,
};

__attribute__((weak)) void irq_default_handler(void)
{
    LOG_ERROR_NUM(_IRQ_ERR_DEFAULT);
    os_alarm_isr();
}

__attribute__((weak)) void irq_exception_handler(void)
{   // Entering the handler for an internal interrupt automatically clears the internal interrupt.
    // Internal interrupts are considered to be non-recoverable in general.
    
    // Ibex can trigger an exception due to the following exception causes:
    
    // 1    Instruction access fault
    // 2    Illegal instruction
    // 3    Breakpoint
    // 5    Load access fault
    // 7    Store access fault
    // 8    Environment call from U-Mode (ECALL)
    // 11   Environment call from M-Mode (ECALL)

    // Read the mcause CSR register
    u32 mcause = 0;
    asm volatile ("csrr %0, mcause" : "=r" (mcause));

    LOG_ERROR_NUM(_IRQ_ERR_EXCEPTION | mcause);

    os_alarm_isr();
    while (1) {}
}

__attribute__((weak)) void irq_nm_handler(void)
{   // Non-maskable interrupt (NMI)
    // This handler may be caused only by ECC error on CPU BUS
    LOG_ERROR_NUM(_IRQ_ERR_NM);
    os_alarm_isr();
}

#define _IRQ_CTRL_REG_READ(offset)             IO_READ_32(CPUSS_IRQ_CTRL_BASE_ADDR+(offset))
#define _IRQ_CTRL_REG_WRITE(offset,value)      IO_WRITE_32(CPUSS_IRQ_CTRL_BASE_ADDR+(offset), value)


///////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt mapping
///////////////////////////////////////////////////////////////////////////////////////////////////
// Configure Interrupt mapping as agreed with FW team:
//  IRQ line    Meaning                                                             Top Channels
//  ----------------------------------------------------------------------------------------------
//  1           Serial subsystem                                                        0,1,2,3
//  2           Flash subsystem                                                         4
//  3           OTP subsystem                                                           5
//  4           Security Center A                                                       7
//  5           Security Center B                                                       8
//  6           Security Center C                                                       9
//  7           Bedrock  PTRNG 1 Interrupt                                              12
//  8           Bedrock  PTRNG 2 Interrupt                                              13
//  9           Bedrock  PUF Interrupt                                                  14
// 10           SPECT (Done and Error Interrupt)                                        16,17
// 11           MAC and Destroy (Done, FLASH read error and Bit Flip error)             18,19,20
// 12           Command processing block                                                21,22
// 13           SCB Secure Channel Block                                                24
// 14           EDB Entropy Distribution Block                                          25
// 15           KDB Key Distribution Block                                              26
// 16           MBIST                                                                   29

void irq_periph_init(void)
{
    // IRQ_0: Serial Subsystem (Request and Response Queue, full, empty, watermarks)
    u32 tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_1_MAP_0_MASK, MAP_IRQ_0) |
              FIELD_PREP(IRQ_CTRL_INT_MAP_1_MAP_1_MASK, MAP_IRQ_0) |
              FIELD_PREP(IRQ_CTRL_INT_MAP_1_MAP_2_MASK, MAP_IRQ_0) |
              FIELD_PREP(IRQ_CTRL_INT_MAP_1_MAP_3_MASK, MAP_IRQ_0);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_1_ADDR, tmp);

    // IRQ 1:   Flash Subsystem
    // IRQ 2:   OTP Subssytem
    // IRQ 3:   Security Center A
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_2_MAP_4_MASK, MAP_IRQ_1) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_2_MAP_6_MASK, MAP_IRQ_2) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_2_MAP_7_MASK, MAP_IRQ_3);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_2_ADDR, tmp);

    // IRQ 4:   Security Center B
    // IRQ 5:   Security Center C
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_3_MAP_8_MASK, MAP_IRQ_4) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_3_MAP_9_MASK, MAP_IRQ_5);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_3_ADDR, tmp);

    // IRQ 6:   PTRNG 1
    // IRQ 7:   PTRNG 2
    // IRQ 8:   PUF
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_4_MAP_12_MASK, MAP_IRQ_6) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_4_MAP_13_MASK, MAP_IRQ_7) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_4_MAP_14_MASK, MAP_IRQ_8);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_4_ADDR, tmp);

    // IRQ 9:   SPECT (Done and Error)
    // IRQ 10:  MACANDD (Done and Flash Read Error)
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_5_MAP_16_MASK, MAP_IRQ_9) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_5_MAP_17_MASK, MAP_IRQ_9) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_5_MAP_18_MASK, MAP_IRQ_10) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_5_MAP_19_MASK, MAP_IRQ_10);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_5_ADDR, tmp);

    // IRQ 10:  MACANDD (Bit Flip Error)
    // IRQ 11:  CPB (Priviledge Check Done, Command not Authorized)
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_6_MAP_20_MASK, MAP_IRQ_10) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_6_MAP_21_MASK, MAP_IRQ_11) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_6_MAP_22_MASK, MAP_IRQ_11);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_6_ADDR, tmp);

    // IRQ 12: Secure Channel Block
    // IRQ 13: Entropy Distribution Block
    // IRQ 14: Key Distribution Block
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_7_MAP_24_MASK, MAP_IRQ_12) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_7_MAP_25_MASK, MAP_IRQ_13) |
          FIELD_PREP(IRQ_CTRL_INT_MAP_7_MAP_26_MASK, MAP_IRQ_14);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_7_ADDR, tmp);

    // IRQ 15: MBIST
    tmp = FIELD_PREP(IRQ_CTRL_INT_MAP_8_MAP_29_MASK, MAP_IRQ_15);

    _IRQ_CTRL_REG_WRITE(IRQ_CTRL_INT_MAP_8_ADDR, tmp);

    // Enable all Interrupt channels mapped as listed above
    cpu_set_bits_csr(CSR_MIE, 
        (1 << CSR_MIE_FIRQ0E ) |
        (1 << CSR_MIE_FIRQ1E ) |
        (1 << CSR_MIE_FIRQ2E ) |
        (1 << CSR_MIE_FIRQ3E ) |
        (1 << CSR_MIE_FIRQ4E ) |
        (1 << CSR_MIE_FIRQ5E ) |
        (1 << CSR_MIE_FIRQ6E ) |
        (1 << CSR_MIE_FIRQ7E ) |
        (1 << CSR_MIE_FIRQ8E ) |
        (1 << CSR_MIE_FIRQ9E ) |
        (1 << CSR_MIE_FIRQ10E) |
        (1 << CSR_MIE_FIRQ11E) |
        (1 << CSR_MIE_FIRQ12E) |
        (1 << CSR_MIE_FIRQ13E) |
        (1 << CSR_MIE_FIRQ14E) |
        (1 << CSR_MIE_MEIE   )
    );

    //Enable global interrupt
    cpu_enable_global_interrupt();
}

//Default handlers it is expected that this overriden if specific handler is implemented

//  0 Serial subsystem handler
__attribute__((weak)) __attribute__((interrupt)) void irq_ss_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ0E);
    FTC_PRINT_MSG("Default irq_ss_handler!!!");
}

//  1 FLASH subsystem
__attribute__((weak)) __attribute__((interrupt)) void irq_flash_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ1E);
    FTC_PRINT_MSG("Default irq_flash_handler!!!");
}

//  2 OTP controller
__attribute__((weak)) __attribute__((interrupt)) void irq_otp_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ2E);
    FTC_PRINT_MSG("Default irq_otp_handler!!!");
}

//  3 Secure center A
__attribute__((weak)) __attribute__((interrupt)) void irq_sc_a_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ3E);
    FTC_PRINT_MSG("Default irq_sc_a_handler!!!");
}

//  4 Secure center B
__attribute__((weak)) __attribute__((interrupt)) void irq_sc_b_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ4E);
    FTC_PRINT_MSG("Default irq_sc_b_handler!!!");
}

//  5 Secure center C
__attribute__((weak)) __attribute__((interrupt)) void irq_sc_c_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ5E);
    FTC_PRINT_MSG("Default irq_sc_c_handler!!!");
}

//  6 Bedrock  PTRNG 1 Interrupt
__attribute__((weak)) __attribute__((interrupt)) void irq_brock_ptrng_1_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ6E);
    FTC_PRINT_MSG("Default irq_brock_ptrng_1_handler!!!");
}

//  7 Bedrock  PTRNG 2 Interrupt
__attribute__((weak)) __attribute__((interrupt)) void irq_brock_ptrng_2_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ7E);
    FTC_PRINT_MSG("Default irq_brock_ptrng_2_handler!!!");
}

//  8 Bedrock  PUF Interrupt
__attribute__((weak)) __attribute__((interrupt)) void irq_brock_puf_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ8E);
    FTC_PRINT_MSG("Default irq_brock_puf_handler!!!");
}

//  9 SPECT (Done and Error Interrupt)
__attribute__((weak)) __attribute__((interrupt)) void irq_spect_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ9E);
    FTC_PRINT_MSG("Default irq_spect_handler!!!");
}

//  10 MAC and Destroy (Done, FLASH read error and Bit Flip error)
__attribute__((weak)) __attribute__((interrupt)) void irq_macandd_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ10E);
    FTC_PRINT_MSG("Default irq_macandd_handler!!!");
}

// 11 Command processing block
__attribute__((weak)) __attribute__((interrupt)) void irq_cpb_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ11E);
    FTC_PRINT_MSG("Default irq_cpb_handler!!!");
}

// 12 Secure channel block
__attribute__((weak)) __attribute__((interrupt)) void irq_scb_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ12E);
    FTC_PRINT_MSG("Default irq_scb_handler!!!");
}

// 13 EDB
__attribute__((weak)) __attribute__((interrupt)) void irq_edb_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ13E);
    FTC_PRINT_MSG("Default irq_edb_handler!!!");
}

// 14 KDB
__attribute__((weak)) __attribute__((interrupt)) void irq_kdb_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_FIRQ14E);
    FTC_PRINT_MSG("Default irq_kdb_handler!!!");
}

// 15 MBIST
__attribute__((weak)) __attribute__((interrupt)) void irq_mbist_handler(void)
{
    cpu_disable_interrupt(CSR_MIE_MEIE);
    FTC_PRINT_MSG("Default irq_mbist_handler!!!");
}

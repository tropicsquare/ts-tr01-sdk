/**
 * @file irq_ctrl.h
 * @author Tropic Square
 * @brief Interrupt controller HW driver header file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef IRQ_CTRL_H
#define IRQ_CTRL_H

#include "common.h"
#include "cpu.h"
#include "csr.h"

/**
 * Enable interrupt
 *
 * @param[in] channel: irq channel
 */
inline void __attribute__((always_inline)) cpu_enable_interrupt(u32 channel)
{
    OS_SANITY_BIT32(channel);
    cpu_set_bits_csr(CSR_MIE, 1 << channel);
}

/**
 * Disable interrupt
 *
 * @param[in] channel: irq channel
 */
inline void __attribute__((always_inline)) cpu_disable_interrupt(u32 channel)
{
    OS_SANITY_BIT32(channel);
    cpu_clear_bits_csr(CSR_MIE, 1 << channel);
}

/**
 * Enable global interrupt
 */
inline void __attribute__((always_inline)) cpu_enable_global_interrupt(void)
{
    cpu_set_bits_csr(CSR_MSTATUS, 1 << CSR_MSTATUS_MIE);
}

/**
 * Disable global interrupt
 */
inline void __attribute__((always_inline)) cpu_disable_global_interrupt(void)
{
    cpu_clear_bits_csr(CSR_MSTATUS, 1 << CSR_MSTATUS_MIE);
}

void irq_periph_init(void);

/**
 * Default interrupt handler
 * @note Will be overridden by handler functions.
 */
void irq_default_handler(void)          __attribute__((interrupt));

void irq_exception_handler(void)        __attribute__((interrupt));

void irq_ss_handler(void)               __attribute__((interrupt));
void irq_flash_handler(void)            __attribute__((interrupt));
void irq_otp_handler(void)              __attribute__((interrupt));
void irq_sc_a_handler(void)             __attribute__((interrupt));
void irq_sc_b_handler(void)             __attribute__((interrupt));
void irq_sc_c_handler(void)             __attribute__((interrupt));
void irq_brock_ptrng_1_handler(void)    __attribute__((interrupt));
void irq_brock_ptrng_2_handler(void)    __attribute__((interrupt));
void irq_brock_puf_handler(void)        __attribute__((interrupt));
void irq_spect_handler(void)            __attribute__((interrupt));
void irq_macandd_handler(void)          __attribute__((interrupt));
void irq_cpb_handler(void)              __attribute__((interrupt));
void irq_scb_handler(void)              __attribute__((interrupt));
void irq_edb_handler(void)              __attribute__((interrupt));
void irq_kdb_handler(void)              __attribute__((interrupt));
void irq_mbist_handler(void)            __attribute__((interrupt));
void irq_nm_handler(void)               __attribute__((interrupt));

#endif  // ! IRQ_CTRL_H

/**
 * @file cpu.h
 * @author Tropic Square
 * @brief Ibex Core register definitions.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */


#ifndef CPU_DEFS_H
#define CPU_DEFS_H

/**
 * Available CPU Control and Status Registers (CSRs)
 */
typedef enum {

    CSR_MSTATUS        = 0x300, /**< 0x300 - mstatus    (r/w): Machine status register */
    CSR_MIE            = 0x304, /**< 0x304 - mie        (r/w): Machine interrupt-enable register */
    CSR_MTVEC          = 0x305, /**< 0x305 - mtvec      (r/w): Machine trap-handler base address (for all traps) */
    
    CSR_MCAUSE         = 0x342, /**< 0x342 - mcause     (r/w): Machine Cause Register*/
    CSR_MTVAL          = 0x343, /**< 0x343 - mtval      (r/w): Machine Trap Value Register */
    CSR_MIP            = 0x344  /**< 0x344 - mip        (r): Machine interrupt-pending register */

} csr_enum_t;

/**
 * CPU <b>mstatus</b> CSR (r/w): Machine status (RISC-V spec.)
 */
enum CSR_MSTATUS_BITS_enum {
    CSR_MSTATUS_MIE   =  3, /**< CPU mstatus CSR  (3): MIE - Machine interrupt enable bit (r/w) */
    CSR_MSTATUS_MPIE  =  7, /**< CPU mstatus CSR  (7): MPIE - Machine previous interrupt enable bit (r/w) */
    CSR_MSTATUS_MPP_L = 11, /**< CPU mstatus CSR (11): MPP_L - Machine previous privilege mode bit low (r/w) */
    CSR_MSTATUS_MPP_H = 12  /**< CPU mstatus CSR (12): MPP_H - Machine previous privilege mode bit high (r/w) */
};

/**
 * CPU <b>mie</b> CSR (r/w): Machine interrupt enable (RISC-V spec.)
 */
enum CSR_MIE_BITS_enum {
    CSR_MIE_MSIE    =  3, /**< CPU mie CSR  (3): MSIE - Machine software interrupt enable (r/w) */
    CSR_MIE_MTIE    =  7, /**< CPU mie CSR  (7): MTIE - Machine timer interrupt enable bit (r/w) */
    CSR_MIE_MEIE    = 11, /**< CPU mie CSR (11): MEIE - Machine external interrupt enable bit (r/w) */

    CSR_MIE_FIRQ0E  = 16, /**< CPU mie CSR (16): FIRQ0E - Fast interrupt channel 0 enable bit (r/w) */
    CSR_MIE_FIRQ1E  = 17, /**< CPU mie CSR (17): FIRQ1E - Fast interrupt channel 1 enable bit (r/w) */
    CSR_MIE_FIRQ2E  = 18, /**< CPU mie CSR (18): FIRQ2E - Fast interrupt channel 2 enable bit (r/w) */
    CSR_MIE_FIRQ3E  = 19, /**< CPU mie CSR (19): FIRQ3E - Fast interrupt channel 3 enable bit (r/w) */
    CSR_MIE_FIRQ4E  = 20, /**< CPU mie CSR (20): FIRQ4E - Fast interrupt channel 4 enable bit (r/w) */
    CSR_MIE_FIRQ5E  = 21, /**< CPU mie CSR (21): FIRQ5E - Fast interrupt channel 5 enable bit (r/w) */
    CSR_MIE_FIRQ6E  = 22, /**< CPU mie CSR (22): FIRQ6E - Fast interrupt channel 6 enable bit (r/w) */
    CSR_MIE_FIRQ7E  = 23, /**< CPU mie CSR (23): FIRQ7E - Fast interrupt channel 7 enable bit (r/w) */
    CSR_MIE_FIRQ8E  = 24, /**< CPU mie CSR (24): FIRQ8E - Fast interrupt channel 8 enable bit (r/w) */
    CSR_MIE_FIRQ9E  = 25, /**< CPU mie CSR (25): FIRQ9E - Fast interrupt channel 9 enable bit (r/w) */
    CSR_MIE_FIRQ10E = 26, /**< CPU mie CSR (26): FIRQ10E - Fast interrupt channel 10 enable bit (r/w) */
    CSR_MIE_FIRQ11E = 27, /**< CPU mie CSR (27): FIRQ11E - Fast interrupt channel 11 enable bit (r/w) */
    CSR_MIE_FIRQ12E = 28, /**< CPU mie CSR (28): FIRQ12E - Fast interrupt channel 12 enable bit (r/w) */
    CSR_MIE_FIRQ13E = 29, /**< CPU mie CSR (29): FIRQ13E - Fast interrupt channel 13 enable bit (r/w) */
    CSR_MIE_FIRQ14E = 30, /**< CPU mie CSR (30): FIRQ14E - Fast interrupt channel 14 enable bit (r/w) */
};

/**
 * CPU <b>mip</b> CSR (r/-): Machine interrupt pending (RISC-V spec.)
 */
enum CSR_MIP_BITS_enum {
    CSR_MIP_MSIP    =  3, /**< CPU mip CSR  (3): MSIP - Machine software interrupt pending (r/-) */
    CSR_MIP_MTIP    =  7, /**< CPU mip CSR  (7): MTIP - Machine timer interrupt pending (r/-) */
    CSR_MIP_MEIP    = 11, /**< CPU mip CSR (11): MEIP - Machine external interrupt pending (r/-) */

    CSR_MIP_FIRQ0P  = 16, /**< CPU mip CSR (16): FIRQ0P - Fast interrupt channel 0 pending (r/-) */
    CSR_MIP_FIRQ1P  = 17, /**< CPU mip CSR (17): FIRQ1P - Fast interrupt channel 1 pending (r/-) */
    CSR_MIP_FIRQ2P  = 18, /**< CPU mip CSR (18): FIRQ2P - Fast interrupt channel 2 pending (r/-) */
    CSR_MIP_FIRQ3P  = 19, /**< CPU mip CSR (19): FIRQ3P - Fast interrupt channel 3 pending (r/-) */
    CSR_MIP_FIRQ4P  = 20, /**< CPU mip CSR (20): FIRQ4P - Fast interrupt channel 4 pending (r/-) */
    CSR_MIP_FIRQ5P  = 21, /**< CPU mip CSR (21): FIRQ5P - Fast interrupt channel 5 pending (r/-) */
    CSR_MIP_FIRQ6P  = 22, /**< CPU mip CSR (22): FIRQ6P - Fast interrupt channel 6 pending (r/-) */
    CSR_MIP_FIRQ7P  = 23, /**< CPU mip CSR (23): FIRQ7P - Fast interrupt channel 7 pending (r/-) */
    CSR_MIP_FIRQ8P  = 24, /**< CPU mip CSR (24): FIRQ8P - Fast interrupt channel 8 pending (r/-) */
    CSR_MIP_FIRQ9P  = 25, /**< CPU mip CSR (25): FIRQ9P - Fast interrupt channel 9 pending (r/-) */
    CSR_MIP_FIRQ10P = 26, /**< CPU mip CSR (26): FIRQ10P - Fast interrupt channel 10 pending (r/-) */
    CSR_MIP_FIRQ11P = 27, /**< CPU mip CSR (27): FIRQ11P - Fast interrupt channel 11 pending (r/-) */
    CSR_MIP_FIRQ12P = 28, /**< CPU mip CSR (28): FIRQ12P - Fast interrupt channel 12 pending (r/-) */
    CSR_MIP_FIRQ13P = 29, /**< CPU mip CSR (29): FIRQ13P - Fast interrupt channel 13 pending (r/-) */
    CSR_MIP_FIRQ14P = 30, /**< CPU mip CSR (30): FIRQ14P - Fast interrupt channel 14 pending (r/-) */
};

#endif // ! CPU_DEFS_H


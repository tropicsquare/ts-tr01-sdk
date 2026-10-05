/**
 * @file arch.h
 * @brief Architecture Abstraction Layer for RISC-V 32-bit.
 *
 * This module provides a set of hardware-specific macros and inline assembly 
 * wrappers to handle low-level operations that are not available in standard C.
 * It includes security-hardened loops, compiler barriers, and register access.
 *
 * @note This file is platform-dependent and designed specifically for 
 *       the RISC-V RV32 architecture using GCC/Clang compilers.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef ARCH_H
#define ARCH_H


/**
 * @brief Opaques the variable x
 *
 * This macro tells the compiler "Do not assume anything about the value of this variable"
 * Used to avoid unwanted optimizations.
 *
 * @param x Variable to opaque
 */
#define ARCH_OPAQUE(x) \
    do {__asm__ volatile ("" : "+r" (x) : : "memory");} while(0)

/**
 * @brief Compiler-only memory barrier.
 * Prevents the compiler from reordering memory accesses across this point.
 */
#define ARCH_COMPILER_BARRIER() \
    do {__asm__ volatile ("" : : : "memory");} while(0)

/**
 * @brief Reads the current value of the Return Address (ra) register.
 * @param dest C variable where the address will be stored.
 */
#define ARCH_READ_RETURN_ADDRESS(dest) \
    do {__asm__ volatile ("mv %0, ra" : "=r" (dest) : : "memory");} while(0)

/**
 * @brief Reads the current value of the Stack Pointer (sp) register.
 *
 * Addresses below the returned value are free stack space, addresses from it up to
 * `__stack_top` belong to the current call frame and to the frames of its callers.
 *
 * @note The "memory" clobber keeps the read from being moved across a memory access,
 *       so in a function which saves `ra` in its prologue the read cannot end up
 *       above the frame setup.
 *
 * @param dest C variable where the address will be stored.
 */
#define ARCH_READ_STACK_POINTER(dest) \
    do {__asm__ volatile ("mv %0, sp" : "=r" (dest) : : "memory");} while(0)

/**
 * @brief Reads the value from a RISC-V Control and Status Register (CSR).
 *
 * @param[in]  reg  The name or address of the CSR register (e.g., `mepc`, `mcause`, `mstatus`).
 *                  This must be a string literal or a symbol recognized by the assembler.
 * @param[out] dest The C variable (typically `u32`) where the result will be stored.
 *
 * @warning The @p reg parameter cannot be passed as a standard dynamic C variable, 
 *          as the assembly instruction requires the register name to be hardcoded.
 */
#define ARCH_READ_CSREG(reg, dest) \
    do {__asm__ volatile  ("csrr %0, " reg : "=r" (dest));} while(0)



/**
 * @brief Write the value to a RISC-V Control and Status Register (CSR).
 *
 * @param[in]  reg  The name or address of the CSR register (e.g., `mepc`, `mcause`, `mstatus`).
 *                  This must be a string literal or a symbol recognized by the assembler.
 * @param[out] val  The C variable (typically `u32`) where the value is stored.
 *
 * @warning The @p reg parameter cannot be passed as a standard dynamic C variable, 
 *          as the assembly instruction requires the register name to be hardcoded.
 */
#define ARCH_WRITE_CSREG(reg, val) \
    do {__asm__ volatile  ("csrw " reg ", %0" : : "r" (val));} while(0)

/**
 * @brief  Hardened infinite loop macro.
 * 
 * This macro creates a resilient infinite loop designed to mitigate Fault Injection 
 * attacks (such as Instruction Skipping or Glitching). 
 *
 * @note We dont use '1' in while() condition to act as returning function for compiler
 *       to keep return address in RA register when using as os_alarm()
 * 
 * @param  loop_body  The C code or function call to execute within the loop.
 */
#define ARCH_SECURE_LOOP(loop_body)  \
    do {                                    \
        __asm__ volatile ("1:");    \
        loop_body;                  \
        __asm__ volatile ("j 1b");  \
        __asm__ volatile ("j 1b");  \
        __asm__ volatile ("j 1b");  \
        __asm__ volatile ("j 1b");  \
    } while (0)


/**
 * @brief Fully hardened conditional loop macro.
 *
 * Two block-scoped C labels declared via __label__:
 *   - <name>_top:  target of the 4× redundant back-jumps emitted via
 *                  'asm goto'.
 *   - <name>_exit: target of the 4× redundant forward jumps emitted via
 *                  'asm goto'.
 *
 * The 4× redundancy in each direction is the FI hardening — a single
 * instruction skip on any one 'j' is absorbed by the next.
 *
 * The "memory" clobber on the back-edge 'asm goto' is the iteration
 * boundary fence: it forces the compiler to treat all memory as
 * potentially modified across the back-jump, so any state cached in
 * registers (timer pointers, loop-invariant addresses, etc.) is
 * reloaded at the top of each iteration. The forward exit jump needs
 * no such fence — code after the macro is ordinary C and emits its own
 * loads from source.
 *
 * @param  name       Identifier used to build unique block-scoped labels
 *                    for this loop. Must be unique within the enclosing
 *                    function.
 * @param  cond       Continue condition; the loop exits when this is false.
 * @param  loop_body  C code or function call to execute each iteration.
 */
#define ARCH_SECURE_WHILE_HARDENED(name, cond, loop_body)              \
    do {                                                               \
        __label__ name##_top;                                          \
        __label__ name##_exit;                                         \
        name##_top:;                                                   \
        if (!(cond)) {                                                 \
            __asm__ goto ("j %l[" #name "_exit]\n\t"                   \
                          "j %l[" #name "_exit]\n\t"                   \
                          "j %l[" #name "_exit]\n\t"                   \
                          "j %l[" #name "_exit]"                       \
                          : : : : name##_exit);                        \
        }                                                              \
        loop_body;                                                     \
        __asm__ goto ("j %l[" #name "_top]\n\t"                        \
                      "j %l[" #name "_top]\n\t"                        \
                      "j %l[" #name "_top]\n\t"                        \
                      "j %l[" #name "_top]"                            \
                      : : : "memory" : name##_top);                    \
        name##_exit:;                                                  \
    } while (0)

/**
 * @brief Insert "nop" instruction.
 */
#define ARCH_NOP() \
    do { __asm__ volatile("nop"); } while (0)

/**
 * @brief Insert "wfi" instruction.
 *
 * Does wait for interrupt.
 */
#define ARCH_WFI() \
    do { __asm__ volatile("wfi" ::: "memory"); } while (0)


/**
 * @brief Call a function and force RA register set.
 * @param name The function name (with quotes)
 */
#define ARCH_CALL_RA(name)                       \
    do {                                         \
      __asm__ volatile ("call " name : : : "ra"); \
    }                                            \
    while (0)


/**
 * @brief Injects a custom label into the generated assembly.
 *
 * This macro serves as a visual anchor for assembly-level analysis. It does not
 * produce any executable instructions, but creates a symbol that persists
 * even through heavy optimizations (like LTO), making the disassembly
 * significantly easier to navigate.
 *
 * @param name The label name as a string literal (e.g., "my_anchor")
 */
#define ARCH_TRACE_LABEL(name)            \
    do {                                  \
        __asm__ volatile (name ":"); \
    }                                     \
    while (0)

#endif // ! ARCH_H


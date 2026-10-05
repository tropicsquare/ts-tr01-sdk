###################################################################
# Boot code
#   1. Place stack at __stack_top symbol
#   2. Initialize GPRs
#   3. Jump to main
#   4. After return from main, keep spinning
#
#  TODO: Define IVT and Interrupt handling!
###################################################################


.section .vectors, "ax"
.option norvc;

# All unimplemented interrupts/exceptions go to the irq_default_handler.
# IBEX core interrupts according to documentation :
#   https://ibex-core.readthedocs.io/en/latest/03_reference/exception_interrupts.html

.org 0x00
jal x0, irq_exception_handler
.rept 6
jal x0, irq_default_handler
.endr
jal x0, irq_timer_handler
.rept 3
jal x0, irq_default_handler
.endr
jal x0, irq_mbist_handler  # 'irq_external' (11) connected to MBIST 
.rept 4
jal x0, irq_default_handler
.endr
# here starts peripheral IRQ table based on irq_periph_init() mapping
jal x0, irq_ss_handler
jal x0, irq_flash_handler
jal x0, irq_otp_handler
jal x0, irq_sc_handler
jal x0, irq_sc_handler
jal x0, irq_sc_handler
jal x0, irq_brock_ptrng_1_handler
jal x0, irq_brock_ptrng_2_handler
jal x0, irq_brock_puf_handler
jal x0, irq_spect_handler
jal x0, irq_macandd_handler
jal x0, irq_cpb_handler
jal x0, irq_scb_handler
jal x0, irq_edb_handler
jal x0, irq_kdb_handler
# last one IRQ 31 is IBEX core NMI
jal x0, irq_nm_handler


.section .init, "ax"
.option rvc;

###############################################################################
# Start of executable code
###############################################################################
.global _reset_vector
_reset_vector:
# Disable maskable interrupts before touching any register. The bootloader
# jumps here with a plain call, leaving MSTATUS[MIE]=1, MIE populated and
# MTVEC still pointing at its own IVT; an interrupt taken during the
# initialization below would dispatch through the bootloader's IVT while SP
# is zeroed, corrupting low memory and clobbering the registers used to set
# up MTVEC and the stack.
# NMI and synchronous exceptions are not maskable and are not covered here.
# Interrupts are re-enabled by irq_periph_init(), reached from main() via
# os_init().
    csrci mstatus, 0x8 # disable global interrupts
    csrw mie, zero     # clear interrupt mask
# Initialize all registers to zero
_init_gprs:
    mv x1, x0
    mv x2, x0
    mv x3, x0
    mv x4, x0
    mv x5, x0
    mv x6, x0
    mv x7, x0
    mv x8, x0
    mv x9, x0
    mv x10, x0
    mv x11, x0
    mv x12, x0
    mv x13, x0
    mv x14, x0
    mv x15, x0
    mv x16, x0
    mv x17, x0
    mv x18, x0
    mv x19, x0
    mv x20, x0
    mv x21, x0
    mv x22, x0
    mv x23, x0
    mv x24, x0
    mv x25, x0
    mv x26, x0
    mv x27, x0
    mv x28, x0
    mv x29, x0
    mv x30, x0
    mv x31, x0

# Initialize MTVEC to point to address where IVT is linked
# This is in case if __vectors_start is placed elsewhere than reset
# value of MTVEC
_set_ivt_base_addr:
    la x1, __vectors_start
    csrrw x0, mtvec, x1
    mv x1, x0


# Initialize stack (Set stack pointer and frame pointer)
_init_stack:
    la sp, __stack_top
    add s0, sp, zero

.global _start
_start:

# Clear RAM space defined in linker script (depends on code destination memory)
_clear_ram:
    la x26, __ram_clear_start
    la x27, __ram_clear_end
    bge x26, x27, _zero_loop_end

_zero_loop:
    sw x0, 0(x26)
    addi x26, x26, 4
    blt x26, x27, _zero_loop
_zero_loop_end:

# Jump to main program
_main_c_function:
    jal main

# If execution ends up here, put the core to sleep
_sleep_loop:
    wfi
    j _sleep_loop


/* testing purposes, put everything to IRAM and allow unlimited acces to DRAM */

INCLUDE "common.ld"; 

OUTPUT_ARCH(riscv)

MEMORY
{
  ivt(rx)    : ORIGIN = IRAM_ADDRESS, LENGTH = IVT_SIZE
  iram (rwx) : ORIGIN = (IRAM_ADDRESS + IVT_SIZE), LENGTH = (IRAM_SIZE - IVT_SIZE)
  dram (rw)  : ORIGIN = DRAM_ADDRESS, LENGTH = (DRAM_SIZE - SHM_SIZE)
  shm (rw)   : ORIGIN = (DRAM_ADDRESS + DRAM_SIZE - SHM_SIZE), LENGTH = SHM_SIZE
}

/* Place stack at the end of instruction RAM */
__stack_top = ORIGIN(iram) + LENGTH(iram);
__memtest_start = ORIGIN(dram);
__memtest_end = ORIGIN(dram) + LENGTH(dram);

/* __ram_clear_start and __ram_clear_end define space cleared by init assembly */
__ram_clear_end =  ORIGIN(iram) + LENGTH(iram);

ENTRY(_reset_vector)

SECTIONS
{
  /* start section on WORD boundary */
  . = ALIGN(4);

  .vectors :
  {
    __vectors_start = .;
    KEEP(*(.vectors))

    . = ALIGN(4);
  } > ivt
  /*
   * Put .init code right at irom start from where IBEX boots.
   * C code (.text) directly follows.
   * The text segment contains program code and read-only data.
   */
  .text :
  {
    *(.init*)
    *(.text*)
    *(.rodata*)
    *(.srodata*)
    _etext = .;

    . = ALIGN(4);
  } > iram

  /*
   * Put data into start of data RAM.
   * TODO: Handle stack overflow!
   */
  .data :
  {
    __data_start = .;
    *(.data*)

    . = ALIGN(4);
    __data_end = .;
  } > iram

  .bss (NOLOAD):
  {
	__ram_clear_start = .;
    __bss_start = .;
    *(.bss*)
    *(.sbss*) /* static variables */
    *(COMMON)

    . = ALIGN(4);
    __bss_end = .;
  } > iram
}

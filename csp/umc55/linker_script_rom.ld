INCLUDE "common.ld";

OUTPUT_ARCH(riscv)

MEMORY
{
  /* IBEX boots from 0x80 offset from boot_address, define dummy ivt for it now. */
  ivt(rx)    : ORIGIN = ROM_ADDRESS, LENGTH = IVT_SIZE
  irom (rx)  : ORIGIN = (ROM_ADDRESS + IVT_SIZE), LENGTH = (ROM_SIZE - IVT_SIZE)
  iram (rwx) : ORIGIN = IRAM_ADDRESS, LENGTH = IRAM_SIZE
  dram (rw)  : ORIGIN = DRAM_ADDRESS, LENGTH = (DRAM_SIZE - SHM_SIZE)
  shm (rw)   : ORIGIN = (DRAM_ADDRESS + DRAM_SIZE - SHM_SIZE), LENGTH = SHM_SIZE
}

/* Place stack at the end of data RAM */
__stack_top = ORIGIN(dram) + LENGTH(dram);

/* Information for bootloader where to put FW */
__fw_start =  ORIGIN(iram);

/*
 * '__ram_clear_start' and '__ram_clear_end' define space cleared by init assembly 
 *   in case ROM code we clear whole dram 
 */
__ram_clear_start =  ORIGIN(dram);
__ram_clear_end =  ORIGIN(dram) + LENGTH(dram);

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
    __rodata_end = .;
  } > irom
  
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
  } > dram

  .bss (NOLOAD):
  {
    __bss_start = .;
    *(.bss*)
    *(.sbss*) /* static variables */
    *(COMMON)

    . = ALIGN(4);
    __bss_end = .;
  } > dram

  /* shared memory space for interaction between bootloader and application */
  .shm (NOLOAD):
  {
    __shm_start = .;
    *(.shm*)
    . = ALIGN(4);
    __shm_end = .;
  } > shm


  .blinfo :
  { /* fixed place for bootloader CRC */
    *(.blinfo)
    . = ALIGN(4);
    __blinfo_end = .;
  } > irom
}

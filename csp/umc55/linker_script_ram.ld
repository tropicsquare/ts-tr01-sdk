INCLUDE "common.ld";

OUTPUT_ARCH(riscv)

MEMORY
{
  ivt(rx)  	 : ORIGIN = IRAM_ADDRESS, LENGTH = IVT_SIZE
  iram (rwx) : ORIGIN = (IRAM_ADDRESS + IVT_SIZE), LENGTH = (IRAM_SIZE - IVT_SIZE)
  dram (rw)  : ORIGIN = DRAM_ADDRESS, LENGTH = (DRAM_SIZE - SHM_SIZE)
  shm (rw)   : ORIGIN = (DRAM_ADDRESS + DRAM_SIZE - SHM_SIZE), LENGTH = SHM_SIZE
}

/* Place stack at the end of data RAM */
__stack_top = ORIGIN(dram) + LENGTH(dram);

/* __ram_clear_start and __ram_clear_end define space cleared by init assembly */
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

    /* finish section on WORD boundary */
    . = ALIGN(4);
  } > ivt

  /* put code in iram */
  .text :
  {
    *(.init*)
    *(.text*)
    *(.rodata*)
    *(.srodata*)
    _etext = .;

    /* finish section on WORD boundary */
    . = ALIGN(4);
    __rodata_end = .;
  } > iram

  /* special section used by IPC driver 
   * it is located at the beginning of DRAM (if used) */
  .ipc :
  {
    __start_ipc = .;
    *(.ipc*)
  } > dram

  /*
   * Put data into start of data RAM.
   * TODO: Handle stack overflow!
   */
  .data :
  {
    __data_start = .;
    *(.data*)

    /* finish section on WORD boundary */
    . = ALIGN(4);
    __data_end = .;
  } > dram

  .bss (NOLOAD):
  {
	__ram_clear_start = .;
    __bss_start = .;
    *(.bss*)
    *(.sbss*) /* static variables */
    *(COMMON)

    /* finish section on WORD boundary */
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
}

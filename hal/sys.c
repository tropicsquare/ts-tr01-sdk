/**
 * @file sys.c
 * @author Tropic Square
 * @brief Basic system tools HAL source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"

#include "cpu.h"
#include "irq_ctrl.h"
#include "io_ops.h"

#include "soc_ctrl.h"

ts_bool sys_init(void)
{
    irq_periph_init();
    soc_ctrl_init();
    return (TS_TRUE);
}

void sys_cpu_sleep(void)
{   // suspend to next interrupt
    asm("wfi");
}

void sys_copy_regs_to_mem(u8 *dest, u32 addr, size_t size)
{
    u32 tmp;

    if (size == 0)
    {
        return;
    }
    OS_SANITY_NULL(dest);
    OS_SANITY_ALIGNED(addr);

    while (size >= sizeof(u32))
    {
        tmp = IO_READ_32(addr);
        memcpy(dest, &tmp, sizeof(u32));
        size -=  sizeof(u32);
        addr += sizeof(u32);
        dest += sizeof(u32);
    }
    if (size) 
    {   // some part of u32 missing
        tmp = IO_READ_32(addr);
        memcpy(dest, &tmp, size);
    }
}

void sys_copy_mem_to_regs(u32 addr, const u8 *src, size_t size)
{
    u32 tmp;

    if (size == 0)
    {
        return;
    }
    OS_SANITY_NULL(src);
    OS_SANITY_ALIGNED(addr);

    while (size >= sizeof(u32))
    {
        memcpy(&tmp, src, sizeof(u32));
        IO_WRITE_32(addr, tmp);

        size -=  sizeof(u32);
        addr += sizeof(u32);
        src += sizeof(u32);
    }
    if (size) 
    {   // some part of u32 missing
        tmp = IO_READ_32(addr);
        memcpy(&tmp, src, size);
        IO_WRITE_32(addr, tmp);
    }
}


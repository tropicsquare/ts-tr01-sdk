/**
 * @file sys.h
 * @author Tropic Square
 * @brief Basic system tools HAL header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SYS_H
#define SYS_H

#include "type.h"

ts_bool sys_init(void);
void sys_cpu_sleep(void);

void sys_copy_regs_to_mem(u8 *dest, u32 addr, size_t size);
void sys_copy_mem_to_regs(u32 addr, const u8 *src, size_t size);

#endif // ! SYS_H

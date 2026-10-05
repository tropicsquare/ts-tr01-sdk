/**
 * @file ipc.c
 * @author Tropic Square
 * @brief IPC (Inter Process communication Channel) driver source file.
 * 
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "ipc.h"

#include "xprintf.h"
#include <stdarg.h>

#define IPC_CPU_TO_PARENT_BUFFER_END (IPC_CPU_TO_PARENT_0_ADDR + IPC_MAX_MSG_SIZE_BYTES)
#define IPC_PARENT_TO_CPU_BUFFER_END (IPC_PARENT_TO_CPU_0_ADDR + IPC_MAX_MSG_SIZE_BYTES)

#ifndef TS_SIMULATION_BUILD
// Do not allocate IPC memory for simulation build. Required portion of DRAM
// will be initialized by TB/FW.
// When allocated, resulting hex file is not able to be loaded by simulation
// because it is supposed to be for IROM/IRAM only and IPC memory is part of
// DRAM.
#define IPC_MEMORY_SIZE 520
u8 ipc_memory[IPC_MEMORY_SIZE] __attribute__((section(".ipc")));

void ipc_init(void)
{
    for (u32 i=0; i<sizeof(ipc_memory); i++)
    {
        ipc_memory[i] = 0;
    }
}
#else
void ipc_init(void)
{
    u32 i;
 
    for (i=0; i<IPC_PARENT_TO_CPU_BUFFER_END; i+=4)
    {
        PTR32_T(IPC_BASE_ADDRESS + i) = 0;
    }
}
#endif // TS_SIMULATION_BUILD


static inline void _wait_for_cpu_done_unset()
{
    // blocks until cpu_done is unset by parent process
    while (1)
    {
        u32 cpu_done = PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) & IPC_MSG_ID_1_CPU_DONE_MASK;

        if (! cpu_done)
        {
            break;
        }
    }
}


static inline void _wait_for_parent_done_set()
{
    // blocks until parent_done is set by parent process
    while (1)
    {
        u32 parent_done = PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_2_ADDR) & IPC_MSG_ID_2_PARENT_DONE_MASK;

        if (parent_done)
        {
            break;
        }
    }
}


static inline void _flush_status_register()
{
    // clear status register for new communication session
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) = 0;
}


void ipc_transmit_data(u32 size, const u32 *data)
{
    if(size > IPC_MAX_MSG_SIZE_WORDS)
    {
        size = IPC_MAX_MSG_SIZE_WORDS;
    }

    _wait_for_cpu_done_unset();

    _flush_status_register();

    // raise CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_BUSY_MASK;

    // set message type to generic
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_GENERIC << IPC_MSG_ID_1_MSG_TYPE_POS);

    u32 buffer_offset = IPC_CPU_TO_PARENT_0_ADDR;

    const u32 *ptr = data;

    for (u32 i = 0 ; i < size ; i ++)
    {
        PTR32_T(IPC_BASE_ADDRESS + buffer_offset) = (u32) * ptr;
        // one int per buffer offset register
        buffer_offset += 4;
        ptr ++;
    }

    // write message size to MSG_SIZE field
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= ((size * 4) << IPC_MSG_ID_1_MSG_SIZE_POS);

    // lower CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) &= ~IPC_MSG_ID_1_CPU_BUSY_MASK;

    // raise CPU_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

    _wait_for_cpu_done_unset();
}

void _ipc_bulk_transfer(u32 size, const u32 *data, bool blocking)
{
    _wait_for_cpu_done_unset();

    _flush_status_register();

    // raise CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_BUSY_MASK;

    // set message type to generic
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_GENERIC << IPC_MSG_ID_1_MSG_TYPE_POS);

    // set message command to bulk transfer
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_BULK_TRANSFER << IPC_MSG_ID_1_MSG_CMD_POS);

    // first word of the message is address of the bulk transfer
    // second word of the message is the size in bytes of the bulk transfer
    u32 bulk_data_address = IPC_CPU_TO_PARENT_0_ADDR;
    u32 bulk_data_size = IPC_CPU_TO_PARENT_0_ADDR + 4;
    PTR32_T(IPC_BASE_ADDRESS + bulk_data_address) = (u32) data;
    PTR32_T(IPC_BASE_ADDRESS + bulk_data_size) = size*4;

    // write message size to MSG_SIZE field - 2 words = 8 bytes
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (8 << IPC_MSG_ID_1_MSG_SIZE_POS);

    // lower CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) &= ~IPC_MSG_ID_1_CPU_BUSY_MASK;

    // raise CPU_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

    if (blocking)
    {
        // will unblock when the bulk transfer is done from the parent side
        _wait_for_cpu_done_unset();
    }
}

void ipc_transmit_data_bulk(u32 size, const u32 *data)
{
    _ipc_bulk_transfer(size, data, true);
}


void ipc_print_data_bulk(u32 size, const u32 *data)
{
    _ipc_bulk_transfer(size, data, false);
}

void ipc_transmit_data_u8(u32 size, const u8 *data)
{
    if(size > IPC_MAX_MSG_SIZE_WORDS)
    {
        size = IPC_MAX_MSG_SIZE_WORDS;
    }

    _wait_for_cpu_done_unset();

    _flush_status_register();

    // raise CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_BUSY_MASK;

    // set message type to generic
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_GENERIC << IPC_MSG_ID_1_MSG_TYPE_POS);

    u32 buffer_offset = IPC_CPU_TO_PARENT_0_ADDR;

    const u8 *ptr = data;

    for (u32 i = 0 ; i < size ; i ++)
    {
        PTR32_T(IPC_BASE_ADDRESS + buffer_offset) = (u8) * ptr;
        // one byte per buffer offset register
        buffer_offset += 4;
        ptr ++;
    }

    // write message size to MSG_SIZE field
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= ((size * 4) << IPC_MSG_ID_1_MSG_SIZE_POS);

    // lower CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) &= ~IPC_MSG_ID_1_CPU_BUSY_MASK;

    // raise CPU_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

    _wait_for_cpu_done_unset();
}


void ipc_receive_data(u32 size, u32 *data)
{
    if(size > IPC_MAX_MSG_SIZE_WORDS)
    {
        size = IPC_MAX_MSG_SIZE_WORDS;
    }

    _wait_for_cpu_done_unset();

    _flush_status_register();

    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_GET_DATA << IPC_MSG_ID_1_MSG_CMD_POS;

    // raise cpu done bit
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

    // wait for cpu done bit lower -> parent has received get_data request
    _wait_for_cpu_done_unset();

    // wait for parent done bit high -> parent has finished writing to buffer
    _wait_for_parent_done_set();

    u32 buffer_address = IPC_PARENT_TO_CPU_0_ADDR;

    for (u32 i = 0; i < size; i ++)
    {
        data[i] = PTR32_T(IPC_BASE_ADDRESS + buffer_address);
        buffer_address += 4;
    }

    // unset PARENT_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_2_ADDR) &= ~IPC_MSG_ID_2_PARENT_DONE_MASK;

}


u32 ipc_receive_num()
{
    u32 number[1];
    ipc_receive_data(1, number);
    return (number[0]);
}


u32 ipc_get_random_num()
{
    _wait_for_cpu_done_unset();

    _flush_status_register();

    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_GET_RND << IPC_MSG_ID_1_MSG_CMD_POS;
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

    _wait_for_parent_done_set();

    u32 random_number = PTR32_T(IPC_BASE_ADDRESS + IPC_PARENT_TO_CPU_0_ADDR);
    // unset PARENT_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_2_ADDR) &= ~IPC_MSG_ID_2_PARENT_DONE_MASK;

    return (random_number);
}


void ipc_print_msg(const char *msg, ...)
{
    _wait_for_cpu_done_unset();
    _flush_status_register();

    // format msg
    char formatted_msg[IPC_MAX_MSG_SIZE_BYTES];
    // By design in the C standard, the opaque va_list type must solely be 
    // initialized by va_start(). Manually initializing it (e.g., = {0}) is 
    // semantically incorrect. The NOLINT suppresses a false positive from 
    // clang-tidy's generalized variable initialization rule.
    va_list val; // NOLINT(cppcoreguidelines-init-variables)
    va_start(val, msg);
    TS_IGNORE_RESULT(xvsnprintf(formatted_msg, IPC_MAX_MSG_SIZE_BYTES, msg, val));
    va_end(val);

    // raise CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_BUSY_MASK;
    // set message type to string
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_STR << IPC_MSG_ID_1_MSG_TYPE_POS);

    u32 buffer_offset = IPC_CPU_TO_PARENT_0_ADDR;

    const char *ch = formatted_msg;
    while(*ch)
    {
        PTR32_T(IPC_BASE_ADDRESS + buffer_offset) = (u32) * ch;
        ch++;
        if(buffer_offset >= IPC_CPU_TO_PARENT_BUFFER_END-1)
        {
            break;
        }
        buffer_offset++;
    }

    // write message size to MSG_SIZE field
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (buffer_offset - IPC_CPU_TO_PARENT_0_ADDR) << IPC_MSG_ID_1_MSG_SIZE_POS;
    // lower CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) &= ~IPC_MSG_ID_1_CPU_BUSY_MASK;
    // raise CPU_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

}


void ipc_print_num(u32 num)
{
    _wait_for_cpu_done_unset();

    _flush_status_register();

    // raise CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_BUSY_MASK;

    // set message type to generic
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= (IPC_MSG_ID_1_GENERIC << IPC_MSG_ID_1_MSG_TYPE_POS);

    // write message size to MSG_SIZE field
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= 1 << IPC_MSG_ID_1_MSG_SIZE_POS;

    // write number to first buffer register IPC_CPU_TO_PARENT_0_ADDR
    PTR32_T(IPC_BASE_ADDRESS + IPC_CPU_TO_PARENT_0_ADDR) = num;

    // lower CPU_BUSY flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) &= ~IPC_MSG_ID_1_CPU_BUSY_MASK;

    // raise CPU_DONE flag
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;
}


void ipc_test_passed(void)
{
    _wait_for_cpu_done_unset();
    
    _flush_status_register();

    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_PASS << IPC_MSG_ID_1_MSG_CMD_POS;
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;

}


void ipc_test_failed(void)
{    
    _wait_for_cpu_done_unset();
    
    _flush_status_register();

    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_FAIL << IPC_MSG_ID_1_MSG_CMD_POS;
    PTR32_T(IPC_BASE_ADDRESS + IPC_MSG_ID_1_ADDR) |= IPC_MSG_ID_1_CPU_DONE_MASK;
}

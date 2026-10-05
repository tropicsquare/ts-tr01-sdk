/**
 * @file shm.c
 * @author Tropic Square
 * @brief Shared Memory HAL source file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "common.h"
#include "shm.h"
#include "crc32.h"


static u32 _shm_data[SHM_SIZE/sizeof(u32)] __attribute__((section(".shm")));
static u32 _shm_crc_value __attribute__((section(".shm")));

static TS_CHECK_RETVAL u32 _shm_crc(void)
{
    return (crc32((u8 *)_shm_data, sizeof(_shm_data)));
}

void shm_init(void)
{
    // init SHM to "all zero" == minimal access
    memset(_shm_data, 0, sizeof(_shm_data));
    _shm_crc_value = _shm_crc();
}

ts_bool shm_ok(void)
{
    if (_shm_crc_value == _shm_crc())
    {
        return TS_TRUE;
    }
    return TS_FALSE;
}

static TS_CHECK_RETVAL ts_bool _offset_valid(u32 offset)
{
    if ((offset >= SHM_SIZE)
     || (offset & 0x3))
    {
        return TS_FALSE;
    }
    return TS_TRUE;
}

u32 shm_read(u32 offset)
{
    if (_offset_valid(offset) != TS_TRUE)
    {
        return 0;
    }
    offset >>= 2;
    return _shm_data[offset];
}

void shm_write_no_crc(u32 offset, u32 value)
{
    if (_offset_valid(offset) != TS_TRUE)
    {
        return;
    }
    offset >>= 2;
    _shm_data[offset] = value;
}

void shm_update_crc(void)
{
    _shm_crc_value = _shm_crc();
}

u32 shm_get_crc(void)
{
    return _shm_crc_value;

}

ts_bool shm_set(u32 offset, u32 value)
{
    if (_offset_valid(offset) != TS_TRUE)
    {
        return TS_FALSE;
    }
    shm_write_no_crc(offset, value);
    shm_update_crc();
    return TS_TRUE;
}

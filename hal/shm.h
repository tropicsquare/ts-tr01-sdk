/**
 * @file shm.h
 * @author Tropic Square
 * @brief Shared Memory HAL hader file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef SHM_H
#define SHM_H

#include "type.h"

/** @brief Data size without CRC. */
#define SHM_SIZE (508)

#define SHM_OFFSET_CONFIGURATION (0)
/** @brief Space reserved to CONFIGURATION_OBJECTS_REGISTERS. */
#define SHM_SIZE_CONFIGURATION (0x180)

/**
 * @name Version defines
 * @note Words 5 - 2 before the end used to store FW versions.
 */
///@{
#define SHM_OFFSET_VERSIONS     (SHM_SIZE - (5 * 4))
#define SHM_SPECT_FW_VERSION    (SHM_OFFSET_VERSIONS + (0 * 4))
#define SHM_SPECT_FW_HASH       (SHM_OFFSET_VERSIONS + (1 * 4))
#define SHM_CPU_FW_VERSION      (SHM_OFFSET_VERSIONS + (2 * 4))
#define SHM_CPU_FW_HASH         (SHM_OFFSET_VERSIONS + (3 * 4))
///@}

/**
 * @name Command defines
 * @note Last word used as command interaction between FW/BL.
 * 
 */
///@{
#define SHM_OFFSET_STATE_COMMAND (SHM_SIZE-4)
    #define SHM_STATE_COMMAND_NONE                 (0)
    #define SHM_STATE_COMMAND_REBOOT      (0x4a781219)
    #define SHM_STATE_COMMAND_MAINTENANCE (0x61b5b495)
    #define SHM_STATE_COMMAND_FORCE_BOOT  (0xe4d1ecbd)
///@}

void shm_init(void);

/**
 * @brief Check shared memory consistency
 * @return the result
 */
ts_bool shm_ok(void);

/**
 * @brief Read one u32 value from shared memory
 *
 * @param[in] offset in bytes
 * @return the result
 */
u32 shm_read(u32 offset);

/**
 * @brief write one u32 value to shared memory without updating CRC
 *
 * Useful for multiple changes, don't forget to update CRC at the end.
 *
 * @param[in] value the value to write
 * @param[in] offset in bytes
 */
void shm_write_no_crc(u32 offset, u32 value);

#define shm_write shm_write_no_crc // backward compatibility define


/**
 * @brief Update CRC for current content.
 */
void shm_update_crc(void);

/**
 * @brief write one u32 value to shared memory and update CRC
 *
 * @param[in] value the value to write
 * @param[in] offset in bytes
 * @return the result
 */
ts_bool shm_set(u32 offset, u32 value);

#endif // ! _SHM_H


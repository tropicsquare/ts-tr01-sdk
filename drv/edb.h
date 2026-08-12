/**
 * @file edb.h
 * @author Tropic Square
 * @brief EDB (Entropy Distribution Block) driver file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef EDB_H
#define EDB_H

#include "type.h"

#include "edb_regs.h"

typedef enum {
    EDB_NORMAL_MODE     = 0x1,
    EDB_DEBUG_MODE      = 0x2,
    EDB_RAW_MODE        = 0x3,

    /** @brief Use EDB_DONT_SET_MODE when you don't want to change the mode configured in HW. */
    EDB_DONT_SET_MODE   = 0xF
} edb_mode_e;

/**
 * @brief EDB configuration structure
 */
typedef struct {
    u32             debug_random_val;
    u16             raw_threshold;
    edb_mode_e      mode:8;
    u8              debug_wait;
    u8              keccak_rounds;
    u8              dummy_rounds;
    u8              absorb_rounds;
} __PACK edb_cfg_t;

/**
 * @brief Enables clock for Entropy Distribution Block
 */
void edb_wakeup(void);

/**
 * @brief Disables clock for Entropy Distribution Block
 */
void edb_suspend(void);

/**
 * @brief Initializes Entropy distribution Block using default configuration
 *
 */
void edb_init(void);

/**
 * @brief Initializes Entropy distribution Block using custom configuration.
 *
 * Alternative to edb_init() for specific purposes.
 *
 * @param edb_cfg Configuration of the block.
 */
void edb_init_cfg(const edb_cfg_t *edb_cfg);

#endif // ! EDB_H


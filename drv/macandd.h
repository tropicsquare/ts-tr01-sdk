/**
 * @file macandd.h
 * @author Tropic Square
 * @brief MACANDD (MAC and Destroy) driver header file.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */



#ifndef MACANDD_H
#define MACANDD_H

#include "type.h"

/**
 * @brief Init subsystem
 */
void macandd_init(void);

/**
 * @brief Enable clock for subsystem
 */
void macandd_wakeup(void);

/**
 * @brief Disable clock for subsystem
 */
void macandd_suspend(void);

/**
 * @brief Execute MAC and Destroy operation
 * This operation uses data prepared in CPB buffer and result data stores also to CPB buffer.
 * @return TS_TRUE when operation done without any error or TS_FALSE otherwise.
 */
ts_bool macandd_exec(void);


/**
 * @brief Clear internal key registers
 */
void macandd_reset(void);


#endif // !MACANDD_H


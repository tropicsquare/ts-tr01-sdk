/**
 * @file ts_rd_bool.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief Redundant bool type definition.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "type.h"
#include "ts_rd_bool.h"
#include "os.h"

const ts_rd_bool TS_RD_TRUE  = {TS_RD_TRUE_W1, TS_RD_TRUE_W2};
const ts_rd_bool TS_RD_FALSE = {TS_RD_FALSE_W1, TS_RD_FALSE_W2};

void ts_rd_bool_assert_valid(ts_rd_bool value)
{
    if ((value.w1 == TS_RD_TRUE_W1) && (value.w2 == TS_RD_TRUE_W2))
    {
        return;
    }
    if ((value.w1 == TS_RD_FALSE_W1) && (value.w2 == TS_RD_FALSE_W2))
    {
        return;
    }
    os_alarm();
}

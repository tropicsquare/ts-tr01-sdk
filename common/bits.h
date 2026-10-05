/**
 * @file bits.h
 * @author Tropic Square
 * @brief Struct and function declarations for dealing with bit assignment.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#ifndef BITS_H
#define BITS_H

/**
 * @brief Number of bits of an unsigned long, the type the masks below are built from.
 * @note Derived from the compiler on purpose: it is 32 for the 32 bit target, but a
 *       host build of the unit tests has a 64 bit long, where a hardcoded 32 would
 *       make GENMASK() produce masks wider than the requested field.
 */
#define BITS_PER_WORD (__CHAR_BIT__ * __SIZEOF_LONG__)

/**
 * @brief Allows token concatenation.
 * @note X = 1 and Y = 10 would return 110.
*/
#define __AC(X,Y)   (X##Y)
#define _AC(X,Y)    __AC(X,Y)

#define _UL(x)      (_AC(x, UL))
#define UL(x)       (_UL(x))

/**
 * @brief BIT defines a bit mask for the specified bit number from 0 to whatever fits into an unsigned long.
 * @note BIT(10) should evaluate to decimal 1024 (which is binary 1 left shifted by 10 bits).
 */
#define BIT(nr) (1UL << (nr))

#define BIT64(nr) (((u64)(1)) << ((u64)(nr)))

#define GENMASK_INPUT_CHECK(h, l) 0

/**
 * h is high index, l is low index in a bitfield.
 * @note __GENMASK returns 32 bit number with 1s in the h-to-l field
          if h = 4 and l = 1, __GENMASK would return 00000000000000000000000000011110.
 * 
 */
#define __GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & \
     (~0UL >> (BITS_PER_WORD - 1 - (h))))

#define GENMASK(h, l) \
    (GENMASK_INPUT_CHECK(h, l) + __GENMASK(h, l))

/* Catches a BITS_PER_WORD which does not match the width of unsigned long - the
   masks would be wider than the requested field, breaking FIELD_GET/FIELD_SET. */
_Static_assert(GENMASK(7, 0) == 0xFFUL, "GENMASK does not match BITS_PER_WORD");
_Static_assert(GENMASK(10, 8) == 0x700UL, "GENMASK does not match BITS_PER_WORD");

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

/**
 * @name Field modifier defines
 * @par Examples
 * \code
 * #define REG_FIELD_A  GENMASK(6, 0)
 * #define REG_FIELD_B  BIT(7)
 * #define REG_FIELD_C  GENMASK(15, 8)
 * #define REG_FIELD_D  GENMASK(31, 16)
 * \endcode
 *
 * Get:
 * \code
 * a = FIELD_GET(REG_FIELD_A, reg);
 * b = FIELD_GET(REG_FIELD_B, reg);
 * \endcode
 *
 * Set:
 * \code
 * reg = FIELD_PREP(REG_FIELD_A, 1) |
 *       FIELD_PREP(REG_FIELD_B, 0) |
 *       FIELD_PREP(REG_FIELD_C, c) |
 *       FIELD_PREP(REG_FIELD_D, 0x40);
 * \endcode
 *
 * Modify:
 * \code
 * FIELD_SET(reg, REG_FIELD_D, 0x40);
 * \endcode
 *
 * @note FIELD_GET and FIELD_PREP are R-value expressions.
 * @note FIELD_SET is statement.
 * @note "({" "})" braces were removed since they are non-ISO C.
 */
///@{
#define FIELD_GET(_mask, _reg)                                  \
    (                                                           \
        (typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask))  \
    )

#define FIELD_PREP(_mask, _val)                                 \
    (                                                           \
        ((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask)    \
    )

#define FIELD_SET(_reg, _mask, _val)      \
    do {                                  \
        _reg &= ~_mask;                   \
        _reg |= FIELD_PREP(_mask, _val);  \
    } while (0)
///@}
#endif // ! BITS_H

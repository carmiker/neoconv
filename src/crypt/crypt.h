/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - shared helpers for the crypto translation units.
 */
#ifndef LITHOGEN_CRYPT_H
#define LITHOGEN_CRYPT_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

/* MAME bitswap<N>(val, bN-1, ..., b0): source bit b_i of val becomes
 * destination bit (N-1-i) counting args left to right.  Ported as a
 * varargs function: nc_bitswap(val, N, bN-1, ..., b0). */
static inline uint32_t nc_bitswap(uint32_t val, int count, ...)
{
    uint32_t out = 0;
    va_list ap;
    int i;
    va_start(ap, count);
    for (i = count - 1; i >= 0; i--) {
        int bit = va_arg(ap, int);
        out |= ((val >> bit) & 1u) << i;
    }
    va_end(ap);
    return out;
}

/* util::sum16_creator::simple - 16-bit byte sum */
static inline uint16_t nc_sum16(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;
    size_t i;
    for (i = 0; i < len; i++)
        sum = (uint16_t)(sum + data[i]);
    return sum;
}

#include "crypt_protos.h"

#endif /* LITHOGEN_CRYPT_H */
#ifndef NC_BIT
#define NC_BIT
#define BIT(x,n) (((x) >> (n)) & 1u)
#endif
/* MAME BYTE_XOR_LE: byte-lane fixup for byte access into 16-bit regions.
 * liblithogen normalizes all 16-bit regions to the little-endian-host MAME
 * layout, where this macro is the identity. */
#ifndef BYTE_XOR_LE
#define BYTE_XOR_LE(a) (a)
#endif

/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - region construction.
 *
 * Replicates MAME's romload semantics (see MAME src/emu/romload.cpp):
 *   1. copy phase: each load writes `group` bytes at a time (reversed
 *      within the group if the load is a _SWAP variant), skipping `skip`
 *      region bytes after each group; ROM_CONTINUE resumes the same file
 *      at a new region offset; ROM_IGNORE discards file bytes.
 *   2. post phase: 16-bit big-endian regions (maincpu) are byte-pair
 *      swapped, emulating a little-endian host.  The result places
 *      standard 16_WORD_SWAP P data in raw file order, which is both the
 *      layout the ported decryption code expects through uint16_t access
 *      on a little-endian machine and the layout the .neo format stores.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lithogen_internal.h"

static int region_index(const nc_game *g, nc_region_kind kind)
{
    int i;
    for (i = 0; i < g->nregions; i++)
        if (g->regions[i].kind == kind)
            return i;
    return -1;
}

/* Region bytes spanned by a load of `len` file bytes in group/skip mode:
 * the trailing skip after the final group is not written. */
static uint32_t load_span(uint32_t len, unsigned group, unsigned skip)
{
    uint32_t ngroups;
    if (len == 0 || group == 0)
        return 0;
    ngroups = (len + group - 1) / group;
    return (ngroups - 1) * (group + skip) + group;
}

static void copy_groups(uint8_t *dst, uint32_t dstoff, uint32_t dstsize,
                        const uint8_t *src, uint32_t len,
                        unsigned group, unsigned skip, int reverse)
{
    uint32_t s = 0;
    uint32_t d = dstoff;
    while (s < len) {
        unsigned g = group;
        unsigned j;
        if (g > len - s)
            g = (unsigned)(len - s);
        for (j = 0; j < g; j++) {
            uint32_t off = d + (reverse ? (g - 1 - j) : j);
            if (off < dstsize)
                dst[off] = src[s + j];
        }
        s += g;
        d += g + skip;
    }
}

int nc_regions_build(const nc_game *g, nc_zipset *zs, nc_regions *r,
                     const lithogen_options *opt, lithogen_report *rep)
{
    int ri;

    uint32_t p_extent = 0, s_extent = 0;
    uint32_t v1_extent = 0, v2_extent = 0;
    uint32_t m_extent = 0, mx_extent = 0;
    uint32_t v2_copy_src = 0xffffffffu;   /* lowest V1 offset copied into V2 */
    int v2_all_copies = -1;               /* -1: no V2 loads seen yet        */
#define TRACK_P(end) do { \
        if (reg->kind == NC_REG_P && (uint32_t)(end) > p_extent) \
            p_extent = (uint32_t)(end); \
        if (reg->kind == NC_REG_S && from_file && (uint32_t)(end) > s_extent) \
            s_extent = (uint32_t)(end); \
        if (reg->kind == NC_REG_M && from_file && (uint32_t)(end) > m_extent) \
            m_extent = (uint32_t)(end); \
        if (reg->kind == NC_REG_MX && from_file && (uint32_t)(end) > mx_extent) \
            mx_extent = (uint32_t)(end); \
        if (reg->kind == NC_REG_V1 && from_file && (uint32_t)(end) > v1_extent) \
            v1_extent = (uint32_t)(end); \
        if (reg->kind == NC_REG_V2 && from_file && (uint32_t)(end) > v2_extent) \
            v2_extent = (uint32_t)(end); \
    } while (0)

    memset(r, 0, sizeof(*r));

    for (ri = 0; ri < g->nregions; ri++) {
        const nc_region *reg = &g->regions[ri];
        /* A few MAME audio descrambles (cthd family, kf2k5uni, matrimbl)
         * address the Z80 region through its mirrored address-space image.
         * Rather than modeling the mirrors, allocate generous scratch and
         * emit only the logical m1-sized image; the consumer regenerates
         * mirrors itself. */
        uint32_t alloc = reg->size;
        uint8_t *buf;
        const char *curfile = NULL;
        uint8_t *fdata = NULL;
        size_t flen = 0, fpos = 0;
        uint16_t li;

        if (reg->kind == NC_REG_M && alloc < 0x90000)
            alloc = 0x90000;
        buf = (uint8_t *)malloc(alloc ? alloc : 1);
        if (!buf) {
            snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX, "out of memory");
            return LITHOGEN_ERR_NOMEM;
        }
        memset(buf, reg->erasefill, alloc);
        r->data[reg->kind] = buf;
        r->size[reg->kind] = reg->size;

        for (li = 0; li < reg->nloads; li++) {
            const nc_load *ld = &g->loads[reg->load0 + li];

            int from_file = (ld->kind == NC_LOAD || ld->kind == NC_CONTINUE);
            if (reg->kind == NC_REG_V2 && ld->kind != NC_COPY)
                v2_all_copies = 0;
            switch ((nc_load_kind)ld->kind) {
            case NC_LOAD: {
                size_t got = 0;
                uint32_t gotcrc = 0;

                if (ld->group == 0) {   /* NO_DUMP: leave erase fill */
                    rep->missing_optional++;
                    free(fdata);
                    fdata = NULL;
                    curfile = NULL;
                    break;
                }
                free(fdata);
                fdata = nc_zipset_read(zs, ld->file, ld->crc, ld->length,
                                       &got, &gotcrc);
                if (!fdata) {
                    if (ld->optional) {
                        nc_warn(opt, "optional ROM %s not found; region"
                                " left empty", ld->file);
                        rep->missing_optional++;
                        curfile = NULL;
                        break;
                    }
                    snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                             "missing ROM: %s (crc %08x) for set %s",
                             ld->file, ld->crc, g->name);
                    return LITHOGEN_ERR_MISSING_ROM;
                }
                curfile = ld->file;
                flen = got;
                fpos = 0;
                if (gotcrc != ld->crc) {
                    rep->crc_mismatches++;
                    nc_warn(opt, "%s: crc %08x, expected %08x",
                            ld->file, gotcrc, ld->crc);
                    if (opt->strict_crc) {
                        snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                                 "CRC mismatch on %s", ld->file);
                        free(fdata);
                        return LITHOGEN_ERR_CRC;
                    }
                }
                if (flen < ld->length)
                    nc_warn(opt, "%s: file is %zu bytes, expected %u",
                            ld->file, flen, ld->length);
                {
                    uint32_t n = ld->length;
                    if (fpos + n > flen)
                        n = (uint32_t)(flen > fpos ? flen - fpos : 0);
                    copy_groups(buf, ld->offset, alloc, fdata + fpos, n,
                                ld->group, ld->skip, ld->reverse);
                    TRACK_P(ld->offset + load_span(n, ld->group, ld->skip));
                    fpos += ld->length;
                }
                break;
            }
            case NC_CONTINUE: {
                uint32_t n = ld->length;
                if (!curfile || !fdata) {
                    snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                             "ROM_CONTINUE without file in %s", g->name);
                    return LITHOGEN_ERR_ARGS;
                }
                if (fpos + n > flen)
                    n = (uint32_t)(flen > fpos ? flen - fpos : 0);
                copy_groups(buf, ld->offset, alloc, fdata + fpos, n,
                            ld->group, ld->skip, ld->reverse);
                TRACK_P(ld->offset + load_span(n, ld->group, ld->skip));
                fpos += ld->length;
                break;
            }
            case NC_RELOAD: {
                uint32_t n = ld->length;
                if (!curfile || !fdata)
                    break;              /* preceding load was optional+absent */
                if (n > flen)
                    n = (uint32_t)flen;
                copy_groups(buf, ld->offset, alloc, fdata, n,
                            ld->group, ld->skip, ld->reverse);
                if (reg->kind == NC_REG_M) {
                    r->m_window_off = ld->offset;
                    r->m_window_len = ld->length;
                }
                break;
            }
            case NC_IGNORE:
                fpos += ld->length;
                break;
            case NC_FILL:
                /* fills initialize RAM windows (jockeygp) or scratch;
                 * they do not extend the populated extent */
                if (ld->offset + ld->length <= reg->size)
                    memset(buf + ld->offset, (int)(ld->crc & 0xff), ld->length);
                break;
            case NC_COPY: {
                /* source region resolved by kind name emitted by gendb */
                nc_region_kind sk = NC_REG_COUNT;
                if (!strcmp(ld->file, "P")) sk = NC_REG_P;
                else if (!strcmp(ld->file, "S")) sk = NC_REG_S;
                else if (!strcmp(ld->file, "M")) sk = NC_REG_M;
                else if (!strcmp(ld->file, "MX")) sk = NC_REG_MX;
                else if (!strcmp(ld->file, "V1")) sk = NC_REG_V1;
                else if (!strcmp(ld->file, "V2")) sk = NC_REG_V2;
                else if (!strcmp(ld->file, "C")) sk = NC_REG_C;
                if (sk == NC_REG_COUNT || !r->data[sk] ||
                    ld->crc + ld->length > r->size[sk] ||
                    ld->offset + ld->length > reg->size) {
                    snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                             "bad ROM_COPY in %s", g->name);
                    return LITHOGEN_ERR_ARGS;
                }
                memcpy(buf + ld->offset, r->data[sk] + ld->crc, ld->length);
                TRACK_P(ld->offset + ld->length);
                if (reg->kind == NC_REG_V2 && sk == NC_REG_V1) {
                    if (v2_all_copies == -1)
                        v2_all_copies = 1;
                    if (ld->crc < v2_copy_src)
                        v2_copy_src = ld->crc;
                } else if (reg->kind == NC_REG_V2) {
                    v2_all_copies = 0;
                }
                break;
            }
            }
        }
        free(fdata);
        (void)region_index;
    }

#undef TRACK_P
    /* round the populated extent up to 64K and record it */
    if (p_extent) {
        p_extent = (p_extent + 0xffffu) & ~0xffffu;
        if (p_extent > r->size[NC_REG_P])
            p_extent = r->size[NC_REG_P];
    }
    if (s_extent) {
        s_extent = (s_extent + 0xffffu) & ~0xffffu;
        if (s_extent > r->size[NC_REG_S])
            s_extent = r->size[NC_REG_S];
    }
    if (m_extent) {
        m_extent = (m_extent + 0xffffu) & ~0xffffu;
        if (m_extent > r->size[NC_REG_M])
            m_extent = r->size[NC_REG_M];
    }
    if (mx_extent) {
        mx_extent = (mx_extent + 0xffffu) & ~0xffffu;
        if (mx_extent > r->size[NC_REG_MX])
            mx_extent = r->size[NC_REG_MX];
    }
    r->p_extent = p_extent;
    r->s_extent = s_extent;
    r->m_extent = m_extent;
    r->mx_extent = mx_extent;

    /* V regions: like P, MAME may declare them at a mapped-window size
     * larger than the file data (diggerma, shocktr2, prototypes); the
     * .neo stores the populated extent. */
    if (v1_extent) {
        v1_extent = (v1_extent + 0xffffu) & ~0xffffu;
        if (v1_extent < r->size[NC_REG_V1])
            r->size[NC_REG_V1] = v1_extent;
    }
    if (v2_extent) {
        v2_extent = (v2_extent + 0xffffu) & ~0xffffu;
        if (v2_extent < r->size[NC_REG_V2])
            r->size[NC_REG_V2] = v2_extent;
    }

    /* Prototype boards (alpham2p, burningfp) share one physical V ROM
     * between both ADPCM buses: MAME loads it at the tail of adpcma and
     * ROM_COPYs it into adpcmb.  The .neo convention stores each file
     * once, so when V2 is entirely a copy of V1's tail, trim V1 to the
     * copy source and let V2 carry the data. */
    if (v2_all_copies == 1 &&
        v2_copy_src + r->size[NC_REG_V2] == r->size[NC_REG_V1])
        r->size[NC_REG_V1] = v2_copy_src;

    /* a cart audiocpu window with no file data (dragonsh) ships as a
     * standard empty 128K M */
    if (m_extent == 0 && !r->data[NC_REG_MX]) {
        if (r->size[NC_REG_M] > 0x20000)
            r->size[NC_REG_M] = 0x20000;
        if (r->data[NC_REG_M])
            memset(r->data[NC_REG_M], 0, r->size[NC_REG_M]);
    }

    /* a set without a usable fixed region (dragonsh) still needs a
     * standard empty S in the output */
    if (!r->data[NC_REG_S] || r->size[NC_REG_S] == 0) {
        free(r->data[NC_REG_S]);
        r->data[NC_REG_S] = (uint8_t *)calloc(0x20000, 1);
        r->size[NC_REG_S] = r->data[NC_REG_S] ? 0x20000 : 0;
    }

    /* post phase: byte swap the 16-bit big-endian program region */
    if (r->data[NC_REG_P]) {
        uint8_t *p = r->data[NC_REG_P];
        uint32_t i, n = r->size[NC_REG_P] & ~1u;
        for (i = 0; i < n; i += 2) {
            uint8_t t = p[i];
            p[i] = p[i + 1];
            p[i + 1] = t;
        }
    }
    return LITHOGEN_OK;
}

void nc_regions_free(nc_regions *r)
{
    int i;
    for (i = 0; i < NC_REG_COUNT; i++) {
        free(r->data[i]);
        r->data[i] = NULL;
    }
}

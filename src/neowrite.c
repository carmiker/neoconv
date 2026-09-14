/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - .neo serialization.
 *
 * Layout (TerraOnion NeoSD; cross-checked against Geolith geo_neo.c):
 *   0x000  'N' 'E' 'O' 0x01
 *   0x004  u32le PSize, SSize, MSize, V1Size, V2Size, CSize
 *   0x01c  u32le Year, Genre, Screenshot, NGH
 *   0x02c  char  Name[33]
 *   0x04d  char  Manufacturer[17]
 *   ...    zero fill to 0x1000
 *   0x1000 P data, S data, M data, V1 data, V2 data, C data
 *
 * P data is stored in raw file order (big-endian byte pairs); the
 * consumer byteswaps at load (geo_m68k_postload / NeoSD firmware).
 * C data is stored c1/c2 byte-interleaved per pair, which is exactly the
 * layout the sprites region already has after LOAD16_BYTE construction.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lithogen_internal.h"

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int wr(FILE *f, const void *data, size_t len)
{
    return len == 0 || fwrite(data, 1, len, f) == len;
}

int nc_neo_write(const nc_game *g, const nc_regions *r,
                 const lithogen_options *opt, const char *outpath,
                 uint32_t *out_ngh, char *err)
{
    uint8_t hdr[4096];
    FILE *f;
    uint32_t psz = r->size[NC_REG_P];
    uint32_t ssz = r->size[NC_REG_S];
    const uint8_t *m_data;
    uint32_t m_size;
    uint32_t ngh;
    uint32_t genre = opt->genre;
    uint32_t screenshot = opt->screenshot;
    const nc_meta *meta = nc_meta_find(g->name);
    const char *name = opt->name_override ? opt->name_override :
        (meta && meta->title) ? meta->title : g->fullname;
    const char *manu = opt->manufacturer_override ? opt->manufacturer_override :
        (meta && meta->manu) ? meta->manu : g->manufacturer;

    /* Known-good set metadata fills genre/screenshot unless the caller
     * overrides them explicitly. */
    if (meta) {
        if (!genre)
            genre = meta->genre;
        if (!screenshot)
            screenshot = meta->screenshot;
    }

    /* MAME declares P regions at the mapped window size (e.g. 1M for a
     * 512K program); the .neo convention stores only the populated data,
     * as consumers mask/mirror themselves. */
    if (r->p_extent && r->p_extent < psz)
        psz = r->p_extent;
    if (r->s_extent && r->s_extent < ssz)
        ssz = r->s_extent;

    /* Encrypted-M1 carts: the shipped M data is the decrypted audiocrypt
     * image; the 0x90000 audiocpu region is only decryption scratch. */
    if (r->data[NC_REG_MX]) {
        /* ship the decrypted image at the original m1 file size */
        m_data = r->data[NC_REG_MX];
        m_size = r->mx_extent ? r->mx_extent : r->size[NC_REG_MX];
    } else if (r->m_window_len &&
               r->m_window_off + r->m_window_len <= r->size[NC_REG_M]) {
        /* load+RELOAD layout: the reload window holds the canonical
         * (and, post-recipe, descrambled) m1 file image */
        m_data = r->data[NC_REG_M] + r->m_window_off;
        m_size = r->m_window_len;
    } else {
        m_data = r->data[NC_REG_M];
        m_size = r->size[NC_REG_M];
        if (r->m_extent && r->m_extent < m_size)
            m_size = r->m_extent;
    }

    /* NGH: 68K word at P offset 0x108 of the (decrypted) program.  The
     * internal P layout reads word values as little-endian byte pairs. */
    ngh = 0;
    if (r->size[NC_REG_P] >= 0x10a)
        ngh = (uint32_t)r->data[NC_REG_P][0x108] |
              ((uint32_t)r->data[NC_REG_P][0x109] << 8);
    if (opt->ngh_override)
        ngh = opt->ngh_override;
    else if (meta && meta->ngh && meta->ngh != ngh) {
        nc_warn(opt, "%s: P ROM NGH %03X differs from known-good set"
                " NGH %03X; using known-good value", g->name, ngh, meta->ngh);
        ngh = meta->ngh;
    }

    if (out_ngh)
        *out_ngh = ngh;

    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'N'; hdr[1] = 'E'; hdr[2] = 'O'; hdr[3] = 0x01;
    put32(hdr + 4,  psz);
    put32(hdr + 8,  ssz);
    put32(hdr + 12, m_size);
    put32(hdr + 16, r->size[NC_REG_V1]);
    put32(hdr + 20, r->size[NC_REG_V2]);
    put32(hdr + 24, r->size[NC_REG_C]);
    put32(hdr + 28, (meta && meta->year > 1978 && meta->year < 2100)
                        ? meta->year : g->year);
    put32(hdr + 32, genre);
    put32(hdr + 36, screenshot);
    put32(hdr + 40, ngh);
    /* Name and Manufacturer are 33- and 17-byte fields with no
     * guaranteed NUL when full (consumers NUL-terminate themselves). */
    strncpy((char *)hdr + 44, name, 33);
    strncpy((char *)hdr + 77, manu, 17);

    f = fopen(outpath, "wb");
    if (!f) {
        snprintf(err, LITHOGEN_ERRSTR_MAX, "cannot open output: %s", outpath);
        return LITHOGEN_ERR_IO;
    }
    if (!wr(f, hdr, sizeof(hdr)) ||
        !wr(f, r->data[NC_REG_P],  psz) ||
        !wr(f, r->data[NC_REG_S],  ssz) ||
        !wr(f, m_data, m_size) ||
        !wr(f, r->data[NC_REG_V1], r->size[NC_REG_V1]) ||
        !wr(f, r->data[NC_REG_V2], r->size[NC_REG_V2]) ||
        !wr(f, r->data[NC_REG_C],  r->size[NC_REG_C])) {
        snprintf(err, LITHOGEN_ERRSTR_MAX, "write failure: %s", outpath);
        fclose(f);
        remove(outpath);
        return LITHOGEN_ERR_IO;
    }
    if (fclose(f) != 0) {
        snprintf(err, LITHOGEN_ERRSTR_MAX, "close failure: %s", outpath);
        return LITHOGEN_ERR_IO;
    }
    return LITHOGEN_OK;
}

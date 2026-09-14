/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - zip archive input (miniz backend).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "miniz.h"
#include "lithogen_internal.h"

struct nc_zipset {
    mz_zip_archive *zips;
    size_t n;
};

static int name_eq(const char *entry, const char *want)
{
    /* compare basename of the entry, case-insensitively */
    const char *base = entry;
    const char *p;
    for (p = entry; *p; p++)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    for (; *base && *want; base++, want++)
        if (tolower((unsigned char)*base) != tolower((unsigned char)*want))
            return 0;
    return *base == '\0' && *want == '\0';
}

nc_zipset *nc_zipset_open(const char *const *paths, size_t npaths, char *err)
{
    nc_zipset *zs = (nc_zipset *)calloc(1, sizeof(*zs));
    size_t i;
    if (!zs)
        return NULL;
    zs->zips = (mz_zip_archive *)calloc(npaths, sizeof(mz_zip_archive));
    if (!zs->zips) {
        free(zs);
        return NULL;
    }
    for (i = 0; i < npaths; i++) {
        if (!mz_zip_reader_init_file(&zs->zips[i], paths[i], 0)) {
            snprintf(err, LITHOGEN_ERRSTR_MAX, "cannot open zip: %s", paths[i]);
            zs->n = i;
            nc_zipset_close(zs);
            return NULL;
        }
    }
    zs->n = npaths;
    return zs;
}

void nc_zipset_close(nc_zipset *zs)
{
    size_t i;
    if (!zs)
        return;
    for (i = 0; i < zs->n; i++)
        mz_zip_reader_end(&zs->zips[i]);
    free(zs->zips);
    free(zs);
}

static uint8_t *extract(mz_zip_archive *za, mz_uint idx, size_t *out_len,
                        uint32_t *out_crc)
{
    mz_zip_archive_file_stat st;
    uint8_t *buf;
    if (!mz_zip_reader_file_stat(za, idx, &st))
        return NULL;
    buf = (uint8_t *)malloc(st.m_uncomp_size ? (size_t)st.m_uncomp_size : 1);
    if (!buf)
        return NULL;
    if (!mz_zip_reader_extract_to_mem(za, idx, buf, (size_t)st.m_uncomp_size, 0)) {
        free(buf);
        return NULL;
    }
    *out_len = (size_t)st.m_uncomp_size;
    *out_crc = (uint32_t)st.m_crc32;
    return buf;
}

uint8_t *nc_zipset_read(nc_zipset *zs, const char *name, uint32_t crc,
                        size_t expect_len, size_t *out_len, uint32_t *out_crc)
{
    size_t z;
    mz_uint i;
    /* pass 1: by name */
    for (z = 0; z < zs->n; z++) {
        mz_zip_archive *za = &zs->zips[z];
        mz_uint nfiles = mz_zip_reader_get_num_files(za);
        for (i = 0; i < nfiles; i++) {
            char fname[512];
            mz_zip_reader_get_filename(za, i, fname, sizeof(fname));
            if (name_eq(fname, name))
                return extract(za, i, out_len, out_crc);
        }
    }
    /* pass 2: by CRC32 (renamed files, split/merged set differences) */
    if (crc) {
        for (z = 0; z < zs->n; z++) {
            mz_zip_archive *za = &zs->zips[z];
            mz_uint nfiles = mz_zip_reader_get_num_files(za);
            for (i = 0; i < nfiles; i++) {
                mz_zip_archive_file_stat st;
                if (!mz_zip_reader_file_stat(za, i, &st))
                    continue;
                if ((uint32_t)st.m_crc32 == crc &&
                    (size_t)st.m_uncomp_size == expect_len)
                    return extract(za, i, out_len, out_crc);
            }
        }
    }
    return NULL;
}

/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - internal definitions.
 * Game data derived from the MAME project (BSD-3-Clause).
 */
#ifndef LITHOGEN_INTERNAL_H
#define LITHOGEN_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "lithogen.h"

/* ------------------------------------------------------------- database -- */

typedef enum {
    NC_REG_P = 0,   /* cslot1:maincpu       (16-bit BE)               */
    NC_REG_S,       /* cslot1:fixed                                    */
    NC_REG_M,       /* cslot1:audiocpu                                 */
    NC_REG_MX,      /* cslot1:audiocrypt                               */
    NC_REG_V1,      /* cslot1:ymsnd:adpcma                             */
    NC_REG_V2,      /* cslot1:ymsnd:adpcmb                             */
    NC_REG_C,       /* cslot1:sprites                                  */
    NC_REG_COUNT
} nc_region_kind;

typedef enum {
    NC_LOAD = 0,    /* load a file (group/skip/reverse semantics)      */
    NC_CONTINUE,    /* continue previous file at a new region offset   */
    NC_IGNORE,      /* skip bytes of the current file                  */
    NC_FILL,        /* fill region bytes (value in crc field)          */
    NC_COPY,        /* copy from another region (region name in file,
                       source offset in crc field)                     */
    NC_RELOAD       /* load the current file again (from its start)    */
} nc_load_kind;

typedef struct {
    const char *file;    /* file name, or source region name for COPY  */
    uint8_t kind;        /* nc_load_kind                               */
    uint32_t offset;     /* destination offset in region               */
    uint32_t length;     /* bytes of file data                         */
    uint32_t crc;        /* expected CRC32 / fill value / copy source  */
    uint8_t group;       /* bytes per group from file (0 = NO_DUMP)    */
    uint8_t skip;        /* bytes skipped in region after each group   */
    uint8_t reverse;     /* reverse bytes within each group            */
    uint8_t optional;    /* missing file is a warning, not an error    */
} nc_load;

typedef struct {
    uint8_t kind;        /* nc_region_kind                             */
    uint32_t size;
    uint8_t erasefill;   /* initial fill value (0x00 or 0xff)          */
    uint16_t load0;      /* first index into the game's load table     */
    uint16_t nloads;
} nc_region;

typedef struct {
    const char *name;
    const char *parent;          /* NULL for parents                   */
    const char *fullname;
    const char *manufacturer;
    uint16_t year;
    const char *cart;            /* cart slot slug -> decrypt recipe   */
    uint8_t nregions;
    const nc_region *regions;
    const nc_load *loads;
} nc_game;

/* route a warning to the caller's log callback or stderr */
void nc_warn(const lithogen_options *opt, const char *fmt, ...);

extern const nc_game nc_games[];
extern const size_t nc_num_games;

/* Per-set header metadata sourced from a known-good .neo set (see
 * tools/gen_meta.py).  Used as defaults for genre/screenshot when the
 * caller does not override them. */
typedef struct {
    const char *name;            /* set name                            */
    uint16_t year;
    uint8_t genre;
    uint16_t screenshot;
    uint32_t ngh;
    const char *title;           /* header Name (max 33 chars)          */
    const char *manu;            /* header Manufacturer (max 17 chars)  */
    /* known-good region sizes P,S,M,V1,V2,C; 0 = unknown */
    uint32_t sizes[6];
} nc_meta;

extern const nc_meta nc_metas[];
extern const size_t nc_num_metas;

const nc_meta *nc_meta_find(const char *set_name);

/* ------------------------------------------------------------ zip input -- */

typedef struct nc_zipset nc_zipset;   /* one or more open zip archives      */

nc_zipset *nc_zipset_open(const char *const *paths, size_t npaths, char *err);
void nc_zipset_close(nc_zipset *zs);
/* Locate an entry by name (case-insensitive, basename match); if absent,
 * fall back to CRC32 match.  Returns malloc'd data or NULL. */
uint8_t *nc_zipset_read(nc_zipset *zs, const char *name, uint32_t crc,
                        size_t expect_len, size_t *out_len, uint32_t *out_crc);

/* ------------------------------------------------------- region building -- */

typedef struct {
    uint8_t *data[NC_REG_COUNT];
    uint32_t size[NC_REG_COUNT];
    /* Highest byte actually populated in P (span-aware, 64K-rounded).
     * The .neo PSize is trimmed to this; recipes still operate on the
     * full MAME region. */
    uint32_t p_extent;
    /* Highest byte covered by actual file loads into S (0 = none).
     * Encrypted sets have no S file data - sfix_decrypt fills the full
     * region - so a zero extent means "keep the declared size". */
    uint32_t s_extent;
    uint32_t m_extent;    /* M file-load extent (0 = none)              */
    /* When the M region uses the load+RELOAD pattern, the canonical m1
     * file image lives at the reload window; the writer emits it. */
    uint32_t m_window_off;
    uint32_t m_window_len;
    uint32_t mx_extent;   /* audiocrypt file extent: encrypted M output */
} nc_regions;

void nc_apply_meta_sizes(const nc_game *g, nc_regions *r,
                         const lithogen_options *opt);

int nc_regions_build(const nc_game *g, nc_zipset *zs, nc_regions *r,
                     const lithogen_options *opt, lithogen_report *rep);
void nc_regions_free(nc_regions *r);

/* ------------------------------------------------------------- recipes --- */

/* Apply the cart's static decryption/descrambling, mirroring the
 * decrypt_all sequence of the corresponding MAME cart device. */
int nc_recipe_apply(const char *cart, nc_regions *r, char *err);
int nc_recipe_known(const char *cart);

/* --------------------------------------------------------------- writer -- */

int nc_neo_write(const nc_game *g, const nc_regions *r,
                 const lithogen_options *opt, const char *outpath,
                 uint32_t *out_ngh, char *err);
const nc_meta *nc_meta_find(const char *set_name);

#endif /* LITHOGEN_INTERNAL_H */

/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - conversion orchestration and public API.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lithogen_internal.h"

static const lithogen_options default_opts; /* zero-initialized */

int lithogen_api_version(void)
{
    return LITHOGEN_API_VERSION;
}

const char *lithogen_version(void)
{
    return LITHOGEN_VERSION;
}

void lithogen_options_init(lithogen_options *opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->struct_size = sizeof(*opt);
}

void nc_warn(const lithogen_options *opt, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (opt && opt->log)
        opt->log(LITHOGEN_LOG_WARN, buf, opt->log_user);
    else
        fprintf(stderr, "warning: %s\n", buf);
}

/* "<dir of zip_path>/<parent>.zip", or NULL if it does not exist */
static char *auto_parent_path(const char *zip_path, const char *parent)
{
    const char *p, *slash = NULL;
    size_t dirlen, n;
    char *out;
    FILE *f;

    for (p = zip_path; *p; p++)
        if (*p == '/' || *p == '\\')
            slash = p;
    dirlen = slash ? (size_t)(slash - zip_path) + 1 : 0;
    n = dirlen + strlen(parent) + 5;
    out = (char *)malloc(n);
    if (!out)
        return NULL;
    memcpy(out, zip_path, dirlen);
    snprintf(out + dirlen, n - dirlen, "%s.zip", parent);
    f = fopen(out, "rb");
    if (!f) {
        free(out);
        return NULL;
    }
    fclose(f);
    return out;
}

static void basename_noext(const char *path, char *out, size_t outsz)
{
    const char *base = path;
    const char *p;
    size_t n;
    for (p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    n = strlen(base);
    if (n > 4 && !strcmp(base + n - 4, ".zip"))
        n -= 4;
    if (n >= outsz)
        n = outsz - 1;
    memcpy(out, base, n);
    out[n] = '\0';
    for (p = out; *p; p++)
        ;
}

const nc_meta *nc_meta_find(const char *set_name)
{
    size_t i;
    for (i = 0; i < nc_num_metas; i++)
        if (!strcmp(nc_metas[i].name, set_name))
            return &nc_metas[i];
    return NULL;
}

/* Early dual-bus carts have a physically separate ADPCM-B ROM, but
 * MAME merges it into the adpcma region (its YM2610 falls back to the
 * merged space for delta-T).  Which carts split, and where, cannot be
 * derived from MAME or from file names (v2/v4 label chaos) - the
 * known-good set is the authority.  When it records a V1/V2 pair whose
 * sum matches our merged chain, split at its boundary.  Other size
 * disagreements are warned, never silently adopted, since resizing
 * P/S/M/C without understanding why would corrupt layout. */
void nc_apply_meta_sizes(const nc_game *g, nc_regions *r,
                         const lithogen_options *opt)
{
    static const char *rn[6] = { "P", "S", "M", "V1", "V2", "C" };
    static const int rk[6] = { NC_REG_P, NC_REG_S, NC_REG_M,
                               NC_REG_V1, NC_REG_V2, NC_REG_C };
    const nc_meta *meta = nc_meta_find(g->name);
    uint32_t ours[6];
    int i;

    if (!meta || !meta->sizes[0])
        return;

    for (i = 0; i < 6; i++)
        ours[i] = r->size[rk[i]];
    if (r->p_extent && r->p_extent < ours[0])
        ours[0] = r->p_extent;
    if (r->s_extent && r->s_extent < ours[1])
        ours[1] = r->s_extent;
    if (r->data[NC_REG_MX])
        ours[2] = r->mx_extent ? r->mx_extent : r->size[NC_REG_MX];
    else if (r->m_window_len &&
             r->m_window_off + r->m_window_len <= r->size[NC_REG_M])
        ours[2] = r->m_window_len;
    else if (r->m_extent && r->m_extent < ours[2])
        ours[2] = r->m_extent;

    /* P: BrezzaSoft-style boards keep a RAM window inside the mapped
     * P range; the known-good size is authoritative as long as it fits
     * the declared region (the buffer holds valid fill either way). */
    if (meta->sizes[0] && meta->sizes[0] != ours[0] &&
        meta->sizes[0] <= r->size[NC_REG_P]) {
        r->p_extent = meta->sizes[0];
        ours[0] = meta->sizes[0];
    }

    /* split a merged V chain at the known-good boundary */
    if (ours[4] == 0 && meta->sizes[4] != 0 &&
        meta->sizes[3] + meta->sizes[4] == ours[3]) {
        uint32_t v1n = meta->sizes[3], v2n = meta->sizes[4];
        uint8_t *v2 = (uint8_t *)malloc(v2n);
        if (v2) {
            memcpy(v2, r->data[NC_REG_V1] + v1n, v2n);
            free(r->data[NC_REG_V2]);
            r->data[NC_REG_V2] = v2;
            r->size[NC_REG_V2] = v2n;
            r->size[NC_REG_V1] = v1n;
            ours[3] = v1n;
            ours[4] = v2n;
        }
    }
    /* shared-bus board with delta-T in use (bjourney, alpham2p): V2
     * carries the whole V image so the game's absolute ADPCM-B pointers
     * resolve through the consumer's 0-based reads; V1 keeps the
     * ADPCM-A part.  The V1 buffer still holds the complete chain even
     * after a tail-copy split (only its logical size was trimmed). */
    else if (meta->sizes[4] == ours[3] + ours[4] &&
             meta->sizes[3] <= ours[3] + ours[4] && meta->sizes[4] != 0) {
        uint32_t total = ours[3] + ours[4];
        uint8_t *v2 = (uint8_t *)calloc(total, 1);
        if (v2) {
            /* delta-T data stays at its absolute offset; the ADPCM-A
             * part below it is zero fill, not a copy */
            memcpy(v2 + meta->sizes[3], r->data[NC_REG_V1] + meta->sizes[3],
                   total - meta->sizes[3]);
            free(r->data[NC_REG_V2]);
            r->data[NC_REG_V2] = v2;
            r->size[NC_REG_V2] = total;
            r->size[NC_REG_V1] = meta->sizes[3];
            ours[4] = total;
            ours[3] = meta->sizes[3];
        }
    }
    /* the V1-tail-copy split (prototype shared V ROM) applies to
     * alpham2p but not burningfp; when the known-good set keeps such a
     * set merged, restore the merged chain (the V1 buffer still holds
     * the tail bytes - only its logical size was trimmed) */
    else if (ours[4] != 0 && meta->sizes[4] == 0 &&
             meta->sizes[3] == ours[3] + ours[4]) {
        r->size[NC_REG_V1] = ours[3] + ours[4];
        free(r->data[NC_REG_V2]);
        r->data[NC_REG_V2] = NULL;
        r->size[NC_REG_V2] = 0;
        ours[3] += ours[4];
        ours[4] = 0;
    }

    for (i = 0; i < 6; i++)
        if (ours[i] != meta->sizes[i])
            nc_warn(opt, "%s: %s size %x differs from known-good %x",
                    g->name, rn[i], ours[i], meta->sizes[i]);
}

int lithogen_find_game(const char *set_name)
{
    size_t i;
    for (i = 0; i < nc_num_games; i++)
        if (!strcmp(nc_games[i].name, set_name))
            return (int)i;
    return -1;
}

size_t lithogen_game_count(void)
{
    return nc_num_games;
}

int lithogen_game_desc_get(size_t index, lithogen_game_desc *desc)
{
    const nc_game *g;
    const nc_meta *m;
    if (index >= nc_num_games || !desc)
        return 0;
    g = &nc_games[index];
    m = nc_meta_find(g->name);
    desc->name = g->name;
    desc->parent = g->parent;
    desc->mame_title = g->fullname;
    desc->mame_manufacturer = g->manufacturer;
    desc->title = (m && m->title) ? m->title : g->fullname;
    desc->manufacturer = (m && m->manu) ? m->manu : g->manufacturer;
    /* the reference set has at least one corrupt year (vliner54: 471);
     * trust it only when plausible */
    desc->year = (m && m->year > 1978 && m->year < 2100) ? m->year : g->year;
    desc->genre = m ? m->genre : 0;
    desc->screenshot = m ? m->screenshot : 0;
    desc->ngh = m ? m->ngh : 0;
    desc->in_reference = (m != NULL);
    return 1;
}

int lithogen_game_info(size_t index, const char **name, const char **parent,
                      const char **fullname, const char **manufacturer,
                      unsigned *year)
{
    if (index >= nc_num_games)
        return 0;
    if (name) *name = nc_games[index].name;
    if (parent) *parent = nc_games[index].parent;
    if (fullname) *fullname = nc_games[index].fullname;
    if (manufacturer) *manufacturer = nc_games[index].manufacturer;
    if (year) *year = nc_games[index].year;
    return 1;
}

const char *lithogen_status_str(lithogen_status s)
{
    switch (s) {
    case LITHOGEN_OK: return "success";
    case LITHOGEN_ERR_ARGS: return "invalid arguments";
    case LITHOGEN_ERR_ZIP: return "cannot open zip archive";
    case LITHOGEN_ERR_UNKNOWN_SET: return "unknown romset";
    case LITHOGEN_ERR_MISSING_ROM: return "missing ROM file";
    case LITHOGEN_ERR_CRC: return "CRC mismatch";
    case LITHOGEN_ERR_RECIPE: return "unsupported cart type";
    case LITHOGEN_ERR_IO: return "output I/O failure";
    case LITHOGEN_ERR_NOMEM: return "out of memory";
    }
    return "unknown error";
}

lithogen_status lithogen_convert(const char *zip_path, const char *out_path,
                               const lithogen_options *opt,
                               lithogen_report *rep)
{
    char setname[64];
    const char **paths = NULL;
    nc_zipset *zs = NULL;
    nc_regions regions;
    char *parent_zip;
    char *bios_zip;
    lithogen_report local_rep;
    int gi;
    size_t npaths, i;
    lithogen_status st;

    if (!opt)
        opt = &default_opts;
    if (!rep)
        rep = &local_rep;
    memset(rep, 0, sizeof(*rep));

    if (!zip_path || (!out_path && !opt->dry_run)) {
        snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX, "NULL path argument");
        return LITHOGEN_ERR_ARGS;
    }

    if (opt->set_name) {
        snprintf(setname, sizeof(setname), "%s", opt->set_name);
    } else {
        basename_noext(zip_path, setname, sizeof(setname));
        for (i = 0; setname[i]; i++)
            setname[i] = (char)tolower((unsigned char)setname[i]);
    }

    gi = lithogen_find_game(setname);
    if (gi < 0) {
        snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                 "unknown romset '%s' (use --set to name one of the %zu"
                 " supported sets)", setname, nc_num_games);
        return LITHOGEN_ERR_UNKNOWN_SET;
    }

    if (!nc_recipe_known(nc_games[gi].cart)) {
        snprintf(rep->errstr, LITHOGEN_ERRSTR_MAX,
                 "internal: no recipe for cart '%s'", nc_games[gi].cart);
        return LITHOGEN_ERR_RECIPE;
    }

    /* Split sets: automatically search "<parent>.zip" alongside the
     * input for ROMs the clone zip does not carry.  Merged and
     * non-merged sets simply never fall through to it. */
    parent_zip = NULL;
    bios_zip = NULL;
    if (!opt->no_auto_parent) {
        if (nc_games[gi].parent)
            parent_zip = auto_parent_path(zip_path, nc_games[gi].parent);
        /* BIOS-side files (SM1 for dev boards) live in neogeo.zip */
        bios_zip = auto_parent_path(zip_path, "neogeo");
    }

    npaths = 1 + (parent_zip ? 1 : 0) + (bios_zip ? 1 : 0) + opt->num_aux_zips;
    paths = (const char **)malloc(npaths * sizeof(*paths));
    if (!paths) {
        free(parent_zip);
        return LITHOGEN_ERR_NOMEM;
    }
    npaths = 0;
    paths[npaths++] = zip_path;
    if (parent_zip)
        paths[npaths++] = parent_zip;
    if (bios_zip)
        paths[npaths++] = bios_zip;
    for (i = 0; i < opt->num_aux_zips; i++)
        paths[npaths++] = opt->aux_zips[i];

    zs = nc_zipset_open(paths, npaths, rep->errstr);
    free((void *)paths);
    free(parent_zip);
    free(bios_zip);
    if (!zs)
        return LITHOGEN_ERR_ZIP;

    st = (lithogen_status)nc_regions_build(&nc_games[gi], zs, &regions, opt, rep);
    nc_zipset_close(zs);
    if (st != LITHOGEN_OK) {
        nc_regions_free(&regions);
        return st;
    }

    if (!opt->dry_run)
        st = (lithogen_status)nc_recipe_apply(nc_games[gi].cart, &regions,
                                             rep->errstr);
    if (st == LITHOGEN_OK && !opt->dry_run) {
        nc_apply_meta_sizes(&nc_games[gi], &regions, opt);
        st = (lithogen_status)nc_neo_write(&nc_games[gi], &regions, opt,
                                          out_path, &rep->ngh, rep->errstr);
    }

    nc_regions_free(&regions);
    return st;
}

#define _POSIX_C_SOURCE 200809L
/* SPDX-License-Identifier: BSD-3-Clause
 * neoconv - reference command line front end for libneoconv.
 *
 * Usage:
 *   neoconv [options] <romset.zip> [more.zip ...]
 *
 * Each named zip is converted to <setname>.neo.  Additional zips given
 * with -p/--parent are searched for ROMs missing from the primary zip
 * (split sets).  See --help.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "neoconv.h"

/* --------- input collection: files, directories, and glob patterns ------ */

typedef struct {
    char **v;
    size_t n, cap;
    int batch;          /* any input came from a dir/glob expansion */
} inputs_t;

static int add_input(inputs_t *in, const char *path) {
    if (in->n == in->cap) {
        size_t ncap = in->cap ? in->cap * 2 : 64;
        char **nv = (char **)realloc(in->v, ncap * sizeof(*nv));
        if (!nv)
            return 0;
        in->v = nv;
        in->cap = ncap;
    }
    in->v[in->n] = strdup(path);
    return in->v[in->n++] != NULL;
}

static int cmp_str(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static int ends_with_zip(const char *name) {
    size_t n = strlen(name);
    return n > 4 && (!strcmp(name + n - 4, ".zip") ||
                     !strcmp(name + n - 4, ".ZIP"));
}

/* Scan `dir` for entries matching `pat` (NULL: any *.zip); add sorted. */
static int scan_dir(inputs_t *in, const char *dir, const char *pat) {
    DIR *d = opendir(dir);
    struct dirent *de;
    size_t first = in->n, i;

    if (!d) {
        fprintf(stderr, "cannot open directory: %s\n", dir);
        return 0;
    }
    while ((de = readdir(d)) != NULL) {
        char path[1024];
        struct stat st;
        if (pat ? (fnmatch(pat, de->d_name, 0) != 0)
                : !ends_with_zip(de->d_name))
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode))
            add_input(in, path);
    }
    closedir(d);
    qsort(in->v + first, in->n - first, sizeof(*in->v), cmp_str);
    i = in->n - first;
    if (i == 0)
        fprintf(stderr, "no matching zip files in %s\n", dir);
    in->batch = 1;
    return i > 0;
}

static int collect_input(inputs_t *in, const char *arg) {
    struct stat st;

    if (stat(arg, &st) == 0 && S_ISDIR(st.st_mode))
        return scan_dir(in, arg, NULL);

    if (strpbrk(arg, "*?[") != NULL) {   /* quoted/unexpanded glob */
        char dir[1024];
        const char *slash = strrchr(arg, '/');
        const char *pat;

        if (slash) {
            size_t n = (size_t)(slash - arg);
            if (n == 0)              /* pattern in the root directory */
                n = 1;
            if (n >= sizeof(dir))
                n = sizeof(dir) - 1;
            memcpy(dir, arg, n);
            dir[n] = '\0';
            pat = slash + 1;
        } else {
            strcpy(dir, ".");
            pat = arg;
        }
        return scan_dir(in, dir, pat);
    }

    return add_input(in, arg);
}

static void usage(void) {
    printf(
"neoconv %s - MAME Neo Geo romset to TerraOnion .neo converter\n"
"\n"
"usage: neoconv [options] <romset.zip | directory | 'glob'> [...]\n"
"\n"
"options:\n"
"  -o, --output <path>     output file (single input) or directory,\n"
"                          created if missing\n"
"  -s, --set <name>        force set name (default: from zip file name)\n"
"  -p, --parent <zip>      additional zip searched for missing ROMs;\n"
"                          may be given multiple times.  The parent of a\n"
"                          split-set clone is found automatically when\n"
"                          <parent>.zip sits beside the input zip\n"
"      --no-auto-parent    disable the automatic parent zip search\n"
"      --strict            fail on CRC mismatch (default: warn)\n"
"      --ngh <hex>         override NGH header field\n"
"      --genre <n>         TerraOnion genre id (0=Other .. 10=Puzzle)\n"
"      --screenshot <n>    NeoSD screenshot id\n"
"      --name <text>       override game name (max 32 chars)\n"
"      --manu <text>       override manufacturer (max 16 chars)\n"
"  -l, --list              list all supported sets and exit\n"
"\n"
"Inputs may be zip files, directories (every contained *.zip is\n"
"converted), or quoted glob patterns. In directory/glob mode, zips\n"
"that are not known romsets (e.g. neogeo.zip) are skipped, not errors.\n"
"  -h, --help              this text\n", NEOCONV_VERSION);
}

static int list_sets(void) {
    size_t i, n = neoconv_game_count();
    for (i = 0; i < n; i++) {
        const char *name, *parent, *full, *manu;
        unsigned year;
        neoconv_game_info(i, &name, &parent, &full, &manu, &year);
        printf("%-12s %-10s %u  %s\n", name, parent ? parent : "-", year, full);
    }
    printf("%zu sets\n", n);
    return 0;
}

int main(int argc, char **argv) {
    neoconv_options opt;
    const char *out = NULL;
    inputs_t in = { NULL, 0, 0, 0 };
    const char *aux[64];
    int i, failures = 0;
    size_t k;

    neoconv_options_init(&opt);
    opt.aux_zips = aux;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage();
            return 0;
        }
        else if (!strcmp(a, "-l") || !strcmp(a, "--list")) {
            return list_sets();
        }
        else if (!strcmp(a, "-o") || !strcmp(a, "--output")) {
            if (++i >= argc)
                goto badarg;
            out = argv[i];
        }
        else if (!strcmp(a, "-s") || !strcmp(a, "--set")) {
            if (++i >= argc)
                goto badarg;
            opt.set_name = argv[i];
        }
        else if (!strcmp(a, "-p") || !strcmp(a, "--parent")) {
            if (++i >= argc || opt.num_aux_zips >= 64) goto badarg;
            aux[opt.num_aux_zips++] = argv[i];
        }
        else if (!strcmp(a, "--no-auto-parent")) {
            opt.no_auto_parent = 1;
        }
        else if (!strcmp(a, "--strict")) {
            opt.strict_crc = 1;
        }
        else if (!strcmp(a, "--ngh")) {
            if (++i >= argc)
                goto badarg;
            opt.ngh_override = (uint32_t)strtoul(argv[i], NULL, 16);
        }
        else if (!strcmp(a, "--genre")) {
            if (++i >= argc)
                goto badarg;
            opt.genre = (uint32_t)strtoul(argv[i], NULL, 0);
        }
        else if (!strcmp(a, "--screenshot")) {
            if (++i >= argc)
                goto badarg;
            opt.screenshot = (uint32_t)strtoul(argv[i], NULL, 0);
        }
        else if (!strcmp(a, "--name")) {
            if (++i >= argc)
                goto badarg;
            opt.name_override = argv[i];
        }
        else if (!strcmp(a, "--manu")) {
            if (++i >= argc)
                goto badarg;
            opt.manufacturer_override = argv[i];
        }
        else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "unknown option: %s\n", a);
            return 2;
        }
        else {
            if (!collect_input(&in, a))
                failures++;
        }
    }
    if (in.n == 0) {
        if (!failures)
            usage();
        return 2;
    }
    if (in.n > 1 && opt.set_name) {
        fprintf(stderr, "--set cannot be combined with multiple inputs\n");
        return 2;
    }
    /* -o names a directory for multiple inputs: create it if missing */
    if (out && (in.n > 1 || strlen(out) < 5 ||
                strcmp(out + strlen(out) - 4, ".neo") != 0))
        (void)mkdir(out, 0777);

    for (k = 0; k < in.n; k++) {
        char outpath[1024];
        neoconv_report rep;
        neoconv_status st;

        const char *input = in.v[k];
        if (out && in.n == 1 && strlen(out) > 4 &&
            !strcmp(out + strlen(out) - 4, ".neo")) {
            snprintf(outpath, sizeof(outpath), "%s", out);
        }
        else {
            /* derive <set>.neo, optionally inside the -o directory */
            const char *base = input, *p;
            char stem[256];
            size_t n;
            for (p = input; *p; p++)
                if (*p == '/' || *p == '\\')
                    base = p + 1;
            n = strlen(base);
            if (n > 4 && !strcmp(base + n - 4, ".zip"))
                n -= 4;
            if (n >= sizeof(stem))
                n = sizeof(stem) - 1;
            memcpy(stem, base, n);
            stem[n] = '\0';
            if (out)
                snprintf(outpath, sizeof(outpath), "%s/%s.neo", out, stem);
            else
                snprintf(outpath, sizeof(outpath), "%s.neo", stem);
        }

        st = neoconv_convert(input, outpath, &opt, &rep);
        if (st == NEOCONV_OK) {
            printf("%s -> %s (NGH %03X)%s\n", input, outpath, rep.ngh,
                   rep.crc_mismatches ? " [CRC warnings]" : "");
        }
        else if (st == NEOCONV_ERR_UNKNOWN_SET && in.batch) {
            printf("%s: skipped (not a known romset)\n", input);
        }
        else {
            fprintf(stderr, "%s: %s: %s\n", input,
                    neoconv_status_str(st), rep.errstr);
            failures++;
        }
    }
    for (k = 0; k < in.n; k++)
        free(in.v[k]);
    free(in.v);
    return failures ? 1 : 0;

badarg:
    usage();
    return 2;
}

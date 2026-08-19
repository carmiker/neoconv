/* SPDX-License-Identifier: BSD-3-Clause
 * libneoconv - convert MAME-layout Neo Geo romsets (.zip) to TerraOnion
 * .neo format, decrypting/descrambling static ROM data as required.
 *
 * ROM layout data and decryption algorithms are derived from the MAME
 * project (BSD-3-Clause).  Runtime protection (SMA banking, PVC, etc.)
 * is the responsibility of the consumer (NeoSD firmware / emulator) and
 * is intentionally not simulated here.
 *
 * Frontend integration notes:
 *  - All strings returned by database queries point at static data and
 *    remain valid for the lifetime of the process.  Strings the caller
 *    passes in are only read during the call.
 *  - The library writes nothing to stdout/stderr when a log callback is
 *    set; without one, warnings go to stderr (CLI behavior).
 *  - Conversions must be serialized: the decryption code (ported from
 *    MAME) keeps translation-unit state, so concurrent neoconv_convert
 *    calls from multiple threads are not supported.  Any single thread,
 *    including a worker thread, is fine.
 *  - File paths are passed to fopen() as-is.  On POSIX systems UTF-8
 *    paths work naturally; Windows frontends should convert paths to
 *    the active code page or add a wide-char I/O layer.
 */
#ifndef NEOCONV_H
#define NEOCONV_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 && defined NEOCONV_SHARED
 #ifdef NEOCONV_BUILD
  #define NEOCONV_API __declspec(dllexport)
 #else
  #define NEOCONV_API __declspec(dllimport)
 #endif
#elif defined __GNUC__ && defined NEOCONV_SHARED
 #define NEOCONV_API __attribute__((visibility("default")))
#else
 #define NEOCONV_API
#endif

#define NEOCONV_VERSION "0.1.0"
/* Bumped whenever the API grows; query at runtime via neoconv_api_version. */
#define NEOCONV_API_VERSION 2
#define NEOCONV_ERRSTR_MAX 256

typedef enum {
    NEOCONV_OK = 0,
    NEOCONV_ERR_ARGS,        /* bad arguments                            */
    NEOCONV_ERR_ZIP,         /* cannot open/read zip archive             */
    NEOCONV_ERR_UNKNOWN_SET, /* zip name does not match a known set      */
    NEOCONV_ERR_MISSING_ROM, /* a required ROM file was not found        */
    NEOCONV_ERR_CRC,         /* CRC mismatch in strict mode              */
    NEOCONV_ERR_RECIPE,      /* unsupported cart type                    */
    NEOCONV_ERR_IO,          /* output I/O failure                       */
    NEOCONV_ERR_NOMEM
} neoconv_status;

typedef enum {
    NEOCONV_LOG_WARN = 0     /* recoverable oddities: CRC mismatch, size
                                disagreement with the reference set,
                                optional ROM absent, NGH cross-check     */
} neoconv_log_level;

/* Receives one complete message per event, without a trailing newline.
 * Called from within neoconv_convert, on the caller's thread. */
typedef void (*neoconv_log_fn)(neoconv_log_level level, const char *msg,
                               void *user);

typedef struct {
    /* Set by neoconv_options_init; lets the library detect callers built
     * against older struct layouts if fields are appended later. */
    size_t struct_size;

    /* Explicit set name; if NULL it is inferred from the zip file name. */
    const char *set_name;
    /* Extra archives to search for ROMs missing from the primary zip
     * (split sets: parent zip, etc.).  "<parent>.zip" and "neogeo.zip"
     * beside the input are probed automatically unless disabled. */
    const char *const *aux_zips;
    size_t num_aux_zips;
    /* Fail on CRC mismatch instead of warning (default: warn). */
    int strict_crc;
    /* Disable the automatic parent/BIOS zip probing. */
    int no_auto_parent;
    /* Validate everything (set identity, ROM presence, CRCs, recipe)
     * but do not decrypt or write the output file. */
    int dry_run;
    /* Warnings are delivered here when set; stderr otherwise. */
    neoconv_log_fn log;
    void *log_user;

    /* Header metadata overrides; 0/NULL = derive automatically (from
     * the known-good table when covered, else from MAME / the decrypted
     * P ROM at 0x108 for NGH). */
    uint32_t ngh_override;
    uint32_t genre;
    uint32_t screenshot;
    const char *name_override;
    const char *manufacturer_override;
} neoconv_options;

typedef struct {
    char errstr[NEOCONV_ERRSTR_MAX];
    unsigned crc_mismatches;
    unsigned missing_optional;   /* optional/NO_DUMP entries skipped     */
    uint32_t ngh;                /* NGH written to the header            */
} neoconv_report;

/* Descriptive record for one supported set. */
typedef struct {
    const char *name;            /* MAME set name                        */
    const char *parent;          /* parent set name, NULL for parents    */
    const char *mame_title;      /* MAME full description                */
    const char *mame_manufacturer;
    const char *title;           /* .neo header Name (reference set when
                                    covered, MAME fullname otherwise)    */
    const char *manufacturer;    /* .neo header Manufacturer             */
    unsigned year;               /* .neo header Year                     */
    unsigned genre;              /* TerraOnion genre id                  */
    unsigned screenshot;         /* NeoSD screenshot id                  */
    uint32_t ngh;                /* 0 when only known post-conversion    */
    int in_reference;            /* covered by the known-good table      */
} neoconv_game_desc;

NEOCONV_API int neoconv_api_version(void);
NEOCONV_API const char *neoconv_version(void);

/* Initialize options to defaults.  Always call this before setting
 * individual fields; it future-proofs against struct growth. */
NEOCONV_API void neoconv_options_init(neoconv_options *opt);

/* Convert one romset zip to a .neo file (or validate it: see dry_run).
 * out_path may be NULL when dry_run is set. */
NEOCONV_API neoconv_status neoconv_convert(const char *zip_path,
                                           const char *out_path,
                                           const neoconv_options *opt,
                                           neoconv_report *rep);

/* Database queries. */
NEOCONV_API size_t neoconv_game_count(void);
NEOCONV_API int neoconv_game_desc_get(size_t index, neoconv_game_desc *desc);
NEOCONV_API int neoconv_find_game(const char *set_name);   /* -1: unknown */

/* Backwards-compatible field query (superseded by neoconv_game_desc_get). */
NEOCONV_API int neoconv_game_info(size_t index, const char **name,
                                  const char **parent, const char **fullname,
                                  const char **manufacturer, unsigned *year);

NEOCONV_API const char *neoconv_status_str(neoconv_status s);

#ifdef __cplusplus
}
#endif

#endif /* NEOCONV_H */

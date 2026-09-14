/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - convert MAME-layout Neo Geo romsets (.zip) to TerraOnion
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
 *    MAME) keeps translation-unit state, so concurrent lithogen_convert
 *    calls from multiple threads are not supported.  Any single thread,
 *    including a worker thread, is fine.
 *  - File paths are passed to fopen() as-is.  On POSIX systems UTF-8
 *    paths work naturally; Windows frontends should convert paths to
 *    the active code page or add a wide-char I/O layer.
 */
#ifndef LITHOGEN_H
#define LITHOGEN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 && defined LITHOGEN_SHARED
 #ifdef LITHOGEN_BUILD
  #define LITHOGEN_API __declspec(dllexport)
 #else
  #define LITHOGEN_API __declspec(dllimport)
 #endif
#elif defined __GNUC__ && defined LITHOGEN_SHARED
 #define LITHOGEN_API __attribute__((visibility("default")))
#else
 #define LITHOGEN_API
#endif

#define LITHOGEN_VERSION "0.1.0"
/* Bumped whenever the API grows; query at runtime via lithogen_api_version. */
#define LITHOGEN_API_VERSION 2
#define LITHOGEN_ERRSTR_MAX 256

typedef enum {
    LITHOGEN_OK = 0,
    LITHOGEN_ERR_ARGS,        /* bad arguments                            */
    LITHOGEN_ERR_ZIP,         /* cannot open/read zip archive             */
    LITHOGEN_ERR_UNKNOWN_SET, /* zip name does not match a known set      */
    LITHOGEN_ERR_MISSING_ROM, /* a required ROM file was not found        */
    LITHOGEN_ERR_CRC,         /* CRC mismatch in strict mode              */
    LITHOGEN_ERR_RECIPE,      /* unsupported cart type                    */
    LITHOGEN_ERR_IO,          /* output I/O failure                       */
    LITHOGEN_ERR_NOMEM
} lithogen_status;

typedef enum {
    LITHOGEN_LOG_WARN = 0     /* recoverable oddities: CRC mismatch, size
                                disagreement with the reference set,
                                optional ROM absent, NGH cross-check     */
} lithogen_log_level;

/* Receives one complete message per event, without a trailing newline.
 * Called from within lithogen_convert, on the caller's thread. */
typedef void (*lithogen_log_fn)(lithogen_log_level level, const char *msg,
                               void *user);

typedef struct {
    /* Set by lithogen_options_init; lets the library detect callers built
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
    lithogen_log_fn log;
    void *log_user;

    /* Header metadata overrides; 0/NULL = derive automatically (from
     * the known-good table when covered, else from MAME / the decrypted
     * P ROM at 0x108 for NGH). */
    uint32_t ngh_override;
    uint32_t genre;
    uint32_t screenshot;
    const char *name_override;
    const char *manufacturer_override;
} lithogen_options;

typedef struct {
    char errstr[LITHOGEN_ERRSTR_MAX];
    unsigned crc_mismatches;
    unsigned missing_optional;   /* optional/NO_DUMP entries skipped     */
    uint32_t ngh;                /* NGH written to the header            */
} lithogen_report;

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
} lithogen_game_desc;

LITHOGEN_API int lithogen_api_version(void);
LITHOGEN_API const char *lithogen_version(void);

/* Initialize options to defaults.  Always call this before setting
 * individual fields; it future-proofs against struct growth. */
LITHOGEN_API void lithogen_options_init(lithogen_options *opt);

/* Convert one romset zip to a .neo file (or validate it: see dry_run).
 * out_path may be NULL when dry_run is set. */
LITHOGEN_API lithogen_status lithogen_convert(const char *zip_path,
                                           const char *out_path,
                                           const lithogen_options *opt,
                                           lithogen_report *rep);

/* Database queries. */
LITHOGEN_API size_t lithogen_game_count(void);
LITHOGEN_API int lithogen_game_desc_get(size_t index, lithogen_game_desc *desc);
LITHOGEN_API int lithogen_find_game(const char *set_name);   /* -1: unknown */

/* Backwards-compatible field query (superseded by lithogen_game_desc_get). */
LITHOGEN_API int lithogen_game_info(size_t index, const char **name,
                                  const char **parent, const char **fullname,
                                  const char **manufacturer, unsigned *year);

LITHOGEN_API const char *lithogen_status_str(lithogen_status s);

#ifdef __cplusplus
}
#endif

#endif /* LITHOGEN_H */

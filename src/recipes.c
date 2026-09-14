/* SPDX-License-Identifier: BSD-3-Clause
 * liblithogen - cart decryption recipes.
 *
 * Each recipe mirrors, call for call, the decrypt_all() sequence of the
 * corresponding MAME cart-slot device (MAME src/devices/bus/neogeo/),
 * covering only the static ROM transformations.  Runtime protection
 * (SMA banking/PRNG, PVC, kof98 vector swap, fatfury2, mslugx, ...) is
 * the consumer's responsibility (see e.g. Geolith geo_m68k.c) and is
 * intentionally not performed here.
 */
#include <stdio.h>
#include <string.h>

#include "lithogen_internal.h"
#include "crypt/crypt.h"

/* GFX keys, as in MAME prot_cmc.h */
#define KOF99_GFX_KEY     (0x00)
#define GAROU_GFX_KEY     (0x06)
#define MSLUG3_GFX_KEY    (0xad)
#define ZUPAPA_GFX_KEY    (0xbd)
#define GANRYU_GFX_KEY    (0x07)
#define S1945P_GFX_KEY    (0x05)
#define PREISLE2_GFX_KEY  (0x9f)
#define BANGBEAD_GFX_KEY  (0xf8)
#define NITD_GFX_KEY      (0xff)
#define SENGOKU3_GFX_KEY  (0xfe)
#define KOF2000_GFX_KEY   (0x00)
#define KOF2001_GFX_KEY   (0x1e)
#define MSLUG4_GFX_KEY    (0x31)
#define ROTD_GFX_KEY      (0x3f)
#define PNYAA_GFX_KEY     (0x2e)
#define KOF2002_GFX_KEY   (0xec)
#define MATRIM_GFX_KEY    (0x6a)
#define SAMSHO5_GFX_KEY   (0x0f)
#define SAMSHO5SP_GFX_KEY (0x0d)
#define MSLUG5_GFX_KEY    (0x19)
#define SVC_GFX_KEY       (0x57)
#define KOF2003_GFX_KEY   (0x9d)
#define JOCKEYGP_GFX_KEY  (0xac)

#define P   r->data[NC_REG_P],  r->size[NC_REG_P]
#define S   r->data[NC_REG_S],  r->size[NC_REG_S]
#define M   r->data[NC_REG_M],  r->size[NC_REG_M]
#define MX  r->data[NC_REG_MX], r->size[NC_REG_MX]
#define V1  r->data[NC_REG_V1], r->size[NC_REG_V1]
#define C   r->data[NC_REG_C],  r->size[NC_REG_C]
#define PBASE r->data[NC_REG_P]

typedef void (*recipe_fn)(nc_regions *r);

/* ----- plain (also covers runtime-only protections) ----------------------*/
static void rc_rom(nc_regions *r) { (void)r; }

/* ----- kof98 --------------------------------------------------------------*/
static void rc_kof98(nc_regions *r) { lithogen_kof98_decrypt_68k(P); }

/* ----- SMA carts ----------------------------------------------------------*/
static void rc_sma_kof99(nc_regions *r) {
    lithogen_kof99_decrypt_68k(PBASE);
    lithogen_cmc42_gfx_decrypt(C, KOF99_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_sma_garou(nc_regions *r) {
    lithogen_garou_decrypt_68k(PBASE);
    lithogen_cmc42_gfx_decrypt(C, GAROU_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_sma_garouh(nc_regions *r) {
    lithogen_garouh_decrypt_68k(PBASE);
    lithogen_cmc42_gfx_decrypt(C, GAROU_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_sma_mslug3(nc_regions *r) {
    lithogen_mslug3_decrypt_68k(PBASE);
    lithogen_cmc42_gfx_decrypt(C, MSLUG3_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_sma_mslug3a(nc_regions *r) {
    lithogen_mslug3a_decrypt_68k(PBASE);
    lithogen_cmc42_gfx_decrypt(C, MSLUG3_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_sma_kof2k(nc_regions *r) {
    lithogen_kof2000_decrypt_68k(PBASE);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, KOF2000_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}

/* ----- CMC42 --------------------------------------------------------------*/
#define CMC42(nm, key) static void rc_cmc42_##nm(nc_regions *r) { \
    lithogen_cmc42_gfx_decrypt(C, key); lithogen_sfix_decrypt(C, S); }
CMC42(zupapa,   ZUPAPA_GFX_KEY)
CMC42(mslug3h,  MSLUG3_GFX_KEY)
CMC42(ganryu,   GANRYU_GFX_KEY)
CMC42(s1945p,   S1945P_GFX_KEY)
CMC42(preisle2, PREISLE2_GFX_KEY)
CMC42(bangbead, BANGBEAD_GFX_KEY)
CMC42(nitd,     NITD_GFX_KEY)
CMC42(sengoku3, SENGOKU3_GFX_KEY)
CMC42(kof99k,   KOF99_GFX_KEY)

/* ----- CMC50 --------------------------------------------------------------*/
#define CMC50(nm, key) static void rc_cmc50_##nm(nc_regions *r) { \
    lithogen_cmc50_m1_decrypt(MX, M); \
    lithogen_cmc50_gfx_decrypt(C, key); lithogen_sfix_decrypt(C, S); }
CMC50(kof2001,  KOF2001_GFX_KEY)
CMC50(kof2000n, KOF2000_GFX_KEY)
CMC50(jockeygp, JOCKEYGP_GFX_KEY)

/* ----- NEO-PCM2 (1999) ----------------------------------------------------*/
static void rc_pcm2_mslug4(nc_regions *r) {
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, MSLUG4_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_decrypt(V1, 8);
}
static void rc_pcm2_ms4p(nc_regions *r) {
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, MSLUG4_GFX_KEY);
    lithogen_pcm2_decrypt(V1, 8);
}
static void rc_pcm2_rotd(nc_regions *r) {
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, ROTD_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_decrypt(V1, 16);
}
static void rc_pcm2_pnyaa(nc_regions *r) {
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, PNYAA_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_decrypt(V1, 4);
}

/* ----- NEO-PVC ------------------------------------------------------------*/
static void rc_pvc_mslug5(nc_regions *r) {
    lithogen_mslug5_decrypt_68k(P);
    lithogen_pcm2_swap(V1, 2);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, MSLUG5_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_pvc_svc(nc_regions *r) {
    lithogen_svc_px_decrypt(P);
    lithogen_pcm2_swap(V1, 3);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, SVC_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_pvc_kf2k3(nc_regions *r) {
    lithogen_kof2003_decrypt_68k(P);
    lithogen_pcm2_swap(V1, 5);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, KOF2003_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}
static void rc_pvc_kf2k3h(nc_regions *r) {
    lithogen_kof2003h_decrypt_68k(P);
    lithogen_pcm2_swap(V1, 5);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, KOF2003_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
}

/* ----- kof2002 family -----------------------------------------------------*/
static void rc_k2k2_kof2k2(nc_regions *r) {
    lithogen_kof2002_decrypt_68k(P);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, KOF2002_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_swap(V1, 0);
}
static void rc_k2k2_kf2k2p(nc_regions *r) {
    lithogen_kof2002_decrypt_68k(P);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, KOF2002_GFX_KEY);
    lithogen_pcm2_swap(V1, 0);
}
static void rc_k2k2_matrim(nc_regions *r) {
    lithogen_matrim_decrypt_68k(P);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, MATRIM_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_swap(V1, 1);
}
static void rc_k2k2_samsh5(nc_regions *r) {
    lithogen_samsho5_decrypt_68k(P);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, SAMSHO5_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_swap(V1, 4);
}
static void rc_k2k2_sams5s(nc_regions *r) {
    lithogen_samsh5sp_decrypt_68k(P);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, SAMSHO5SP_GFX_KEY);
    lithogen_sfix_decrypt(C, S);
    lithogen_pcm2_swap(V1, 6);
}

/* ----- bootlegs -----------------------------------------------------------*/
static void rc_boot_cthd2k3(nc_regions *r) {
    lithogen_decrypt_cthd2003(C, M, S);
    lithogen_patch_cthd2003(P);
}
static void rc_boot_ct2k3sp(nc_regions *r) {
    lithogen_decrypt_ct2k3sp(C, M, S);
    lithogen_patch_cthd2003(P);
}
static void rc_boot_ct2k3sa(nc_regions *r) {
    lithogen_decrypt_ct2k3sa(C, M);
    lithogen_patch_ct2k3sa(P);
}
static void rc_boot_matrimbl(nc_regions *r) {
    lithogen_matrim_decrypt_68k(P);
    lithogen_sfix_decrypt(C, S);      /* required for text layer */
    lithogen_matrimbl_decrypt(C, M);
}
static void rc_boot_kf2k2b(nc_regions *r) {
    lithogen_kof2002_decrypt_68k(P);
    lithogen_pcm2_swap(V1, 0);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_kof2002b_gfx_decrypt(r->data[NC_REG_C], 0x4000000);
    lithogen_kof2002b_gfx_decrypt(r->data[NC_REG_S], 0x20000);
}
static void rc_boot_kf2k2mp(nc_regions *r) {
    lithogen_kf2k2mp_decrypt(P);
    lithogen_pcm2_swap(V1, 0);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_sx_decrypt(S, 2);
    lithogen_cmc50_gfx_decrypt(C, KOF2002_GFX_KEY);
}
static void rc_boot_kf2k2mp2(nc_regions *r) {
    lithogen_kf2k2mp2_px_decrypt(P);
    lithogen_pcm2_swap(V1, 0);
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_sx_decrypt(S, 1);
    lithogen_cmc50_gfx_decrypt(C, KOF2002_GFX_KEY);
}
static void rc_boot_kf2k3bl(nc_regions *r) {
    lithogen_cmc50_gfx_decrypt(C, KOF2003_GFX_KEY);
    lithogen_pcm2_swap(V1, 5);
    lithogen_sx_decrypt(S, 1);
}
static void rc_boot_kf2k3pl(nc_regions *r) {
    lithogen_cmc50_gfx_decrypt(C, KOF2003_GFX_KEY);
    lithogen_pcm2_swap(V1, 5);
    lithogen_kf2k3pl_px_decrypt(P);
    lithogen_sx_decrypt(S, 1);
}
static void rc_boot_kf2k3upl(nc_regions *r) {
    lithogen_cmc50_gfx_decrypt(C, KOF2003_GFX_KEY);
    lithogen_pcm2_swap(V1, 5);
    lithogen_kf2k3upl_px_decrypt(P);
    lithogen_sx_decrypt(S, 2);
}
static void rc_boot_kf10th(nc_regions *r) {
    lithogen_kof10th_decrypt(P);
    /* MAME's 0x40000 fixed region is a runtime banking window; the FIX
     * data is generated dynamically from P by the consumer's board
     * emulation.  Ship a standard empty 128K S. */
    if (r->size[NC_REG_S] > 0x20000)
        r->size[NC_REG_S] = 0x20000;
}
static void rc_boot_garoubl(nc_regions *r) {
    lithogen_sx_decrypt(S, 2);
    lithogen_cx_decrypt(C);
}
static void rc_boot_kof97oro(nc_regions *r) {
    lithogen_kof97oro_px_decode(P);
    lithogen_sx_decrypt(S, 1);
    lithogen_cx_decrypt(C);
}
static void rc_boot_kf10thep(nc_regions *r) {
    lithogen_kf10thep_px_decrypt(P);
    lithogen_sx_decrypt(S, 1);
}
static void rc_boot_kf2k5uni(nc_regions *r) {
    lithogen_kf2k5uni_px_decrypt(P);
    lithogen_kf2k5uni_sx_decrypt(S);
    lithogen_kf2k5uni_mx_decrypt(M);
}
static void rc_boot_kf2k4se(nc_regions *r) {
    lithogen_decrypt_kof2k4se_68k(P);
}
static void rc_boot_lans2004(nc_regions *r) {
    lithogen_lans2004_decrypt_68k(P);
    lithogen_lans2004_vx_decrypt(V1);
    lithogen_sx_decrypt(S, 1);
    lithogen_cx_decrypt(C);
}
static void rc_boot_samsho5b(nc_regions *r) {
    lithogen_samsho5b_px_decrypt(P);
    lithogen_samsho5b_vx_decrypt(V1);
    lithogen_sx_decrypt(S, 1);
    lithogen_cx_decrypt(C);
}
static void rc_boot_mslug3b6(nc_regions *r) {
    lithogen_sx_decrypt(S, 2);
    lithogen_cmc42_gfx_decrypt(C, MSLUG3_GFX_KEY);
}
static void rc_boot_ms5plus(nc_regions *r) {
    lithogen_cmc50_m1_decrypt(MX, M);
    lithogen_cmc50_gfx_decrypt(C, MSLUG5_GFX_KEY);
    lithogen_pcm2_swap(V1, 2);
    lithogen_sx_decrypt(S, 1);
}
static void rc_boot_mslug5b(nc_regions *r) {
    lithogen_mslug5b_vx_decrypt(V1);
    lithogen_sx_decrypt(S, 2);
    lithogen_mslug5b_cx_decrypt(C);
}
static void rc_boot_kog(nc_regions *r) {
    lithogen_kog_px_decrypt(P);
    lithogen_sx_decrypt(S, 1);
    lithogen_cx_decrypt(C);
}

/* boot_svc M regions arrive in MAME's Z80 address-space arrangement
 * ([fileB][fileB-mirror][fileA]); the .neo ships the descrambled 128K
 * logical rom ([B][A]) - the file halves swapped, undoing the bootleg
 * board's m1 address-line inversion. */
static void svc_m1_fileimage(nc_regions *r) {
    uint8_t *m = r->data[NC_REG_M];
    if (r->size[NC_REG_M] < 0x30000)
        return;
    memcpy(m + 0x10000, m + 0x20000, 0x10000);  /* [B][A] */
    r->size[NC_REG_M] = 0x20000;
}
static void rc_boot_svcboot(nc_regions *r) {
    svc_m1_fileimage(r);
    lithogen_svcboot_px_decrypt(P);
    lithogen_svcboot_cx_decrypt(C);
}
static void rc_boot_svcplus(nc_regions *r) {
    svc_m1_fileimage(r);
    lithogen_svcplus_px_decrypt(P);
    lithogen_svcboot_cx_decrypt(C);
    lithogen_sx_decrypt(S, 1);
    lithogen_svcplus_px_hack(P);
}
static void rc_boot_svcplusa(nc_regions *r) {
    svc_m1_fileimage(r);
    lithogen_svcplusa_px_decrypt(P);
    lithogen_svcboot_cx_decrypt(C);
    lithogen_svcplus_px_hack(P);
}
static void rc_boot_svcsplus(nc_regions *r) {
    svc_m1_fileimage(r);
    lithogen_svcsplus_px_decrypt(P);
    lithogen_sx_decrypt(S, 2);
    lithogen_svcboot_cx_decrypt(C);
    lithogen_svcsplus_px_hack(P);
}
static void rc_boot_sbp(nc_regions *r) {
    /* MAME sbp.cpp patches a word at 0x1200 at init; like kof10th's
     * Altera overlays this is runtime behavior the consumer's board
     * handling covers, so the .neo ships the unpatched program. */
    (void)r;
}

static const struct { const char *slug; recipe_fn fn; } recipes[] = {
    { "rom", rc_rom },
    { "rom_vliner", rc_rom },        /* runtime board (BrezzaSoft)         */
    { "rom_fatfur2", rc_rom },       /* runtime protection                 */
    { "rom_mslugx", rc_rom },        /* runtime protection                 */
    { "rom_kof98", rc_kof98 },
    { "sma_kof99", rc_sma_kof99 },
    { "sma_garou", rc_sma_garou },
    { "sma_garouh", rc_sma_garouh },
    { "sma_mslug3", rc_sma_mslug3 },
    { "sma_mslug3a", rc_sma_mslug3a },
    { "sma_kof2k", rc_sma_kof2k },
    { "cmc42_zupapa", rc_cmc42_zupapa },
    { "cmc42_mslug3h", rc_cmc42_mslug3h },
    { "cmc42_ganryu", rc_cmc42_ganryu },
    { "cmc42_s1945p", rc_cmc42_s1945p },
    { "cmc42_preisle2", rc_cmc42_preisle2 },
    { "cmc42_bangbead", rc_cmc42_bangbead },
    { "cmc42_nitd", rc_cmc42_nitd },
    { "cmc42_sengoku3", rc_cmc42_sengoku3 },
    { "cmc42_kof99k", rc_cmc42_kof99k },
    { "cmc50_kof2001", rc_cmc50_kof2001 },
    { "cmc50_kof2000n", rc_cmc50_kof2000n },
    { "cmc50_jockeygp", rc_cmc50_jockeygp },
    { "pcm2_mslug4", rc_pcm2_mslug4 },
    { "pcm2_ms4p", rc_pcm2_ms4p },
    { "pcm2_rotd", rc_pcm2_rotd },
    { "pcm2_pnyaa", rc_pcm2_pnyaa },
    { "pvc_mslug5", rc_pvc_mslug5 },
    { "pvc_svc", rc_pvc_svc },
    { "pvc_kf2k3", rc_pvc_kf2k3 },
    { "pvc_kf2k3h", rc_pvc_kf2k3h },
    { "k2k2_kof2k2", rc_k2k2_kof2k2 },
    { "k2k2_kf2k2p", rc_k2k2_kf2k2p },
    { "k2k2_matrim", rc_k2k2_matrim },
    { "k2k2_samsh5", rc_k2k2_samsh5 },
    { "k2k2_sams5s", rc_k2k2_sams5s },
    { "boot_cthd2k3", rc_boot_cthd2k3 },
    { "boot_ct2k3sp", rc_boot_ct2k3sp },
    { "boot_ct2k3sa", rc_boot_ct2k3sa },
    { "boot_matrimbl", rc_boot_matrimbl },
    { "boot_kf2k2b", rc_boot_kf2k2b },
    { "boot_kf2k2mp", rc_boot_kf2k2mp },
    { "boot_kf2k2mp2", rc_boot_kf2k2mp2 },
    { "boot_kf2k3bl", rc_boot_kf2k3bl },
    { "boot_kf2k3pl", rc_boot_kf2k3pl },
    { "boot_kf2k3upl", rc_boot_kf2k3upl },
    { "boot_kf10th", rc_boot_kf10th },
    { "boot_kf10thep", rc_boot_kf10thep },
    { "boot_kf2k5uni", rc_boot_kf2k5uni },
    { "boot_kf2k4se", rc_boot_kf2k4se },
    { "boot_garoubl", rc_boot_garoubl },
    { "boot_kof97oro", rc_boot_kof97oro },
    { "boot_lans2004", rc_boot_lans2004 },
    { "boot_samsho5b", rc_boot_samsho5b },
    { "boot_mslug3b6", rc_boot_mslug3b6 },
    { "boot_ms5plus", rc_boot_ms5plus },
    { "boot_mslug5b", rc_boot_mslug5b },
    { "boot_kog", rc_boot_kog },
    { "boot_svcboot", rc_boot_svcboot },
    { "boot_svcplus", rc_boot_svcplus },
    { "boot_svcplusa", rc_boot_svcplusa },
    { "boot_svcsplus", rc_boot_svcsplus },
    { "boot_sbp", rc_boot_sbp },
};

int nc_recipe_known(const char *cart)
{
    size_t i;
    for (i = 0; i < sizeof(recipes) / sizeof(recipes[0]); i++)
        if (!strcmp(recipes[i].slug, cart))
            return 1;
    return 0;
}

int nc_recipe_apply(const char *cart, nc_regions *r, char *err)
{
    size_t i;
    for (i = 0; i < sizeof(recipes) / sizeof(recipes[0]); i++) {
        if (!strcmp(recipes[i].slug, cart)) {
            recipes[i].fn(r);
            return LITHOGEN_OK;
        }
    }
    snprintf(err, LITHOGEN_ERRSTR_MAX, "unsupported cart type: %s", cart);
    return LITHOGEN_ERR_RECIPE;
}

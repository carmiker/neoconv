#!/usr/bin/env python3
"""Port MAME Neo Geo protection/crypto sources (BSD-3-Clause) to ISO C99.

Extracts static tables verbatim and device member functions from the MAME
sources, strips class scoping and C++ constructs, and emits one C file per
source into src/crypt/. The C compiler is the final verifier.
"""
import re, sys, os, pathlib

MAME = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "ref/mame/src/devices/bus/neogeo")
OUT = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else "src/crypt")
OUT.mkdir(parents=True, exist_ok=True)

# file -> (class name, exported functions, extra file-scope decls)
FILES = {
    "prot_cmc.cpp": dict(
        cls="cmc_prot_device",
        export=["cmc42_gfx_decrypt", "cmc50_gfx_decrypt", "sfix_decrypt", "cmc50_m1_decrypt"],
        skip=["kof99_neogeo_gfx_decrypt", "kof2000_neogeo_gfx_decrypt"],
        prelude=(
            "/* member table pointers -> translation-unit state */\n"
            "static const uint8_t *type0_t03, *type0_t12, *type1_t03, *type1_t12;\n"
            "static const uint8_t *address_8_15_xor1, *address_8_15_xor2;\n"
            "static const uint8_t *address_16_23_xor1, *address_16_23_xor2;\n"
            "static const uint8_t *address_0_7_xor;\n"),
    ),
    "prot_sma.cpp": dict(cls="sma_prot_device",
        export=["kof99_decrypt_68k", "garou_decrypt_68k", "garouh_decrypt_68k",
                "mslug3_decrypt_68k", "mslug3a_decrypt_68k", "kof2000_decrypt_68k"],
        skip=[]),
    "prot_pcm2.cpp": dict(cls="pcm2_prot_device", export=["decrypt", "swap"], skip=[],
        rename={"decrypt": "pcm2_decrypt", "swap": "pcm2_swap"}),
    "prot_pvc.cpp": dict(cls="pvc_prot_device",
        export=["mslug5_decrypt_68k", "svc_px_decrypt", "kof2003_decrypt_68k", "kof2003h_decrypt_68k"],
        skip=[]),
    "prot_kof98.cpp": dict(cls="kof98_prot_device", export=["decrypt_68k"], skip=[],
        rename={"decrypt_68k": "kof98_decrypt_68k"},
        prelude="static uint16_t m_default_rom[2];\n"),
    "prot_kof2k2.cpp": dict(cls="kof2002_prot_device",
        export=["kof2002_decrypt_68k", "matrim_decrypt_68k", "samsho5_decrypt_68k", "samsh5sp_decrypt_68k"],
        skip=[]),
    "prot_cthd.cpp": dict(cls="cthd_prot_device",
        export=["decrypt_cthd2003", "patch_cthd2003", "decrypt_ct2k3sp", "decrypt_ct2k3sa",
                "patch_ct2k3sa", "matrimbl_decrypt"],
        skip=[]),
    # Functions whose tail bakes in patches that real hardware applies as
    # a runtime overlay (Altera CPLD); the .neo consumer's board emulation
    # performs those, so the converter must not.
    "prot_misc.cpp": dict(cls="neoboot_prot_device",
        strip_after={"kof10th_decrypt": "// Altera protection chip patches"},
        export=["cx_decrypt", "sx_decrypt", "kof97oro_px_decode", "kf10thep_px_decrypt",
                "kf2k5uni_px_decrypt", "kf2k5uni_sx_decrypt", "kf2k5uni_mx_decrypt",
                "decrypt_kof2k4se_68k", "lans2004_vx_decrypt", "lans2004_decrypt_68k",
                "samsho5b_px_decrypt", "samsho5b_vx_decrypt",
                "mslug5b_px_decrypt", "mslug5b_vx_decrypt", "mslug5b_cx_decrypt",
                "kog_px_decrypt", "svcboot_px_decrypt", "svcboot_cx_decrypt",
                "svcplus_px_decrypt", "svcplus_px_hack", "svcplusa_px_decrypt",
                "svcsplus_px_decrypt", "svcsplus_px_hack",
                "kf2k2mp_decrypt", "kf2k2mp2_px_decrypt", "kof2002b_gfx_decrypt",
                "kof10th_decrypt"],
        skip=[]),
    "prot_kof2k3bl.cpp": dict(cls="kof2k3bl_prot_device",
        export=["pl_px_decrypt", "upl_px_decrypt"], skip=[],
        prelude="static uint16_t m_overlay; /* runtime member; value re-read by protection sim */\n",
        rename={"pl_px_decrypt": "kf2k3pl_px_decrypt", "upl_px_decrypt": "kf2k3upl_px_decrypt"}),
}

HEADER = """/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME ({src}) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

"""

FUNC_RE = re.compile(
    r"^(?P<ret>(?:static\s+)?(?:void|int|uint8_t|uint16_t|uint32_t))\s+"
    r"(?P<cls>[a-z0-9_]+_device)::(?P<name>[a-z0-9_]+)\s*\((?P<args>[^)]*)\)\s*\n\{",
    re.M)

def extract_body(text, start):
    """start points at the opening '{'. Return (body_including_braces, end_index)."""
    depth = 0
    i = start
    while i < len(text):
        c = text[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return text[start:i + 1], i + 1
        i += 1
    raise RuntimeError("unbalanced braces")

def c99ify(code):
    # bitswap<N>(v, ...) -> nc_bitswap(v, N, ...)
    code = re.sub(r"bitswap<(\d+)>\s*\(\s*", lambda m: "nc_bitswap(%s, " % m.group(1), code)
    # after moving N: nc_bitswap(N, v, ...) has wrong arg order; fix: we inserted N first.
    # We want nc_bitswap(v, N, ...). Easier: match again and swap.
    code = re.sub(r"nc_bitswap\((\d+), ([^,]+?),", r"nc_bitswap(\2, \1,", code)
    # std::vector<T> name(size);  -> T *name = (T *)calloc(size, sizeof(T)); NC_FREE(name)
    code = re.sub(
        r"std::vector<\s*(uint8_t|uint16_t|uint32_t)\s*>\s+([a-z0-9_]+)\s*\(([^;]+)\);",
        r"\1 *\2 = (\1 *)calloc((size_t)(\3), sizeof(\1)); /*NCVEC:\2*/", code)
    code = code.replace("nullptr", "NULL")
    code = code.replace("&buf[0]", "buf")
    code = code.replace("&buffer[0]", "buffer")
    code = code.replace("&tmp[0]", "tmp")
    code = code.replace("&dst[0]", "dst")
    # util::sum16
    code = re.sub(r"util::sum16_creator::simple\(([^,]+),\s*([^)]+)\)", r"nc_sum16(\1, \2)", code)
    # strip debug dump blocks: if (0) { ... }
    code = re.sub(r"\n\tif \(0\)\n\t\{(?:[^{}]|\{[^{}]*\})*\}\n", "\n", code)
    # size_t / const fine. u32 etc not used. logerror shouldn't appear in ported fns.
    return code

def add_frees(body):
    """Insert free() for each NCVEC-marked allocation at the end of its
    enclosing block (before the matching close brace)."""
    while True:
        m = re.search(r"/\*NCVEC:([a-z0-9_]+)\*/", body)
        if not m:
            return body
        name = m.group(1)
        # find close brace of the block containing the marker
        depth = 0
        i = m.end()
        while i < len(body):
            c = body[i]
            if c == '{':
                depth += 1
            elif c == '}':
                if depth == 0:
                    break
                depth -= 1
            i += 1
        body = body[:i] + ("free(%s);\n\t" % name) + body[i:]
        body = body[:m.start()] + body[m.end():]

protos = []
for fname, spec in FILES.items():
    src = (MAME / fname).read_text()
    cls = spec["cls"]
    rename = spec.get("rename", {})
    out_parts = [HEADER.format(src=fname)]
    if spec.get("prelude"):
        out_parts.append(spec["prelude"] + "\n")

    # 1) static const tables + file-scope statics: copy every top-level
    #    "static const ... = { ... };" block verbatim.
    for m in re.finditer(r"^static const [^=\n]+=\s*\n?\{", src, re.M):
        body, _ = extract_body(src, src.index("{", m.start()))
        decl = src[m.start():src.index("{", m.start())]
        out_parts.append(decl + body + ";\n\n")

    # 1b) top-level #define helper macros used by the algorithms
    for line in src.splitlines():
        if line.startswith("#define ") and "GFX_KEY" not in line:
            out_parts.append(c99ify(line) + "\n\n")

    # 2) member functions
    for m in FUNC_RE.finditer(src):
        if m.group("cls") != cls:
            continue
        name = m.group("name")
        if name in spec["skip"] or name in ("device_start", "device_reset"):
            continue
        brace = src.index("{", m.end() - 1)
        body, _ = extract_body(src, brace)
        # strip disabled debug dump blocks before content filtering
        body = re.sub(r"\n\tif \(0\)\n\t\{(?:[^{}]|\{[^{}]*\})*\}\n", "\n", body)
        blob = m.group("args") + body
        if any(t in blob for t in ("offs_t", "m_bankdev", "m_cart_ram", "machine()",
                                   "logerror", "save_item", "m_fixed[", "m_sma_rgn", "m_sma_rng")):
            continue
        marker = spec.get("strip_after", {}).get(name)
        if marker and marker in body:
            body = body[:body.index(marker)].rstrip() + "\n}"
        exported = name in spec["export"]
        newname = rename.get(name, name)
        args = m.group("args")
        ret = m.group("ret").replace("static ", "")
        sig = "%s %s(%s)" % (ret, ("lithogen_" + newname) if exported else newname, args)
        code = c99ify(sig + "\n" + body)
        code = add_frees(code)
        if not exported:
            code = "static " + code
        else:
            protos.append(sig.replace("lithogen_" + newname, "lithogen_" + newname) + ";")
        out_parts.append(code + "\n\n")

    # rewrite intra-file calls to renamed/exported functions
    text = "".join(out_parts)
    for name in spec["export"]:
        newname = "lithogen_" + rename.get(name, name)
        # call sites: plain "name(" not already prefixed and not the definition
        text = re.sub(r"(?<![a-z0-9_])%s\(" % re.escape(name),
                      newname + "(", text)
        # fix the definition line (was double-prefixed by the sub above? no:
        # definition already uses lithogen_ name so the regex can't match it)
    outname = fname.replace(".cpp", ".c").replace("prot_", "nc_")
    (OUT / outname).write_text(text)
    print("wrote", OUT / outname, len(text), "bytes")

# emit prototypes header fragment
(OUT / "crypt_protos.h").write_text(
    "/* generated by port_crypto.py */\n" + "\n".join(sorted(set(protos))) + "\n")
print("wrote", OUT / "crypt_protos.h")

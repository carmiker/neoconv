#!/usr/bin/env python3
"""Generate src/games_db.c for liblithogen from MAME's src/mame/snk/neogeo.cpp.

Parses ROM_START blocks (with the Neo Geo helper macros expanded), GAME()
lines, and the machine_config -> cart-slot bindings, and emits C tables.
MAME is BSD-3-Clause; the generated data is derived from it.
"""
import re, sys, pathlib

SRC = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "ref/mame/src/mame/snk/neogeo.cpp")
OUT = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else "src/games_db.c")
text = SRC.read_text()

# ---------------------------------------------------------------- regions ---
REGION_MAP = {
    "cslot1:maincpu": "P", "maincpu": "P",
    "cslot1:fixed": "S", "fixed": "S",
    "cslot1:audiocpu": "M", "audiocpu": "M",
    "cslot1:audiocrypt": "MX", "audiocrypt": "MX",
    "cslot1:ymsnd:adpcma": "V1", "ymsnd:adpcma": "V1",
    "cslot1:ymsnd:adpcmb": "V2", "ymsnd:adpcmb": "V2",
    "cslot1:sprites": "C", "sprites": "C",
}
SKIP_REGIONS = {"mainbios", "fixedbios", "spritegen:zoomy"}
AUDIOBIOS = "audiobios"

def expand_macros(body):
    """Expand the neogeo.cpp helper macros into plain ROM_* statements."""
    out = []
    for line in body.splitlines():
        st = line.strip()
        m = re.match(r'NEO_BIOS_AUDIO_(64|128|256|512)K\(\s*"([^"]+)"\s*,\s*(?:BAD_DUMP\s+)?CRC\(([0-9a-fA-F]+)\)', st)
        if m:
            # faithful to neogeo.h: the file is loaded at 0 and reloaded
            # at 0x10000, so the canonical file image sits at 0x10000
            size = {"64": 0x10000, "128": 0x20000, "256": 0x40000, "512": 0x80000}[m.group(1)]
            out.append('ROM_REGION( 0x%x, "cslot1:audiocpu", 0 )' % (0x10000 + size))
            out.append('ROM_LOAD( "%s", 0x00000, 0x%x, CRC(%s) )' % (m.group(2), size, m.group(3)))
            out.append('ROM_RELOAD( 0x10000, 0x%x )' % size)
            continue
        m = re.match(r'NEO_BIOS_AUDIO_ENCRYPTED_(128|256|512)K\(\s*"([^"]+)"\s*,\s*(?:BAD_DUMP\s+)?CRC\(([0-9a-fA-F]+)\)', st)
        if m:
            size = {"128": 0x20000, "256": 0x40000, "512": 0x80000}[m.group(1)]
            out.append('ROM_REGION( 0x90000, "cslot1:audiocpu", ROMREGION_ERASEFF )')
            out.append('ROM_REGION( 0x80000, "cslot1:audiocrypt", 0 )')
            out.append('ROM_LOAD( "%s", 0x00000, 0x%x, CRC(%s) )' % (m.group(2), size, m.group(3)))
            continue
        m = re.match(r'NEO_SFIX_(64|128)K\(\s*"([^"]+)"\s*,\s*(?:BAD_DUMP\s+)?CRC\(([0-9a-fA-F]+)\)', st)
        if m:
            size = {"64": 0x10000, "128": 0x20000}[m.group(1)]
            out.append('ROM_REGION( 0x20000, "cslot1:fixed", 0 )')
            out.append('ROM_LOAD( "%s", 0x000000, 0x%x, CRC(%s) )' % (m.group(2), size, m.group(3)))
            continue
        if st.startswith(("NEOGEO_BIOS", "ROM_Y_ZOOM", "ROM_DEFAULT_BIOS",
                          "NEO_JAPAN_BIOS", "ROM_SYSTEM_BIOS", "ROM_LOAD16_WORD_SWAP_BIOS")):
            continue
        out.append(st)
    return out

LOAD_KINDS = {
    # macro -> (group, skip, reverse)
    "ROM_LOAD":              (1, 0, 0),
    "ROM_LOAD16_BYTE":       (1, 1, 0),
    "ROM_LOAD32_BYTE":       (1, 3, 0),
    "ROM_LOAD16_WORD":       (2, 0, 0),
    "ROM_LOAD16_WORD_SWAP":  (2, 0, 1),
    "ROM_LOAD32_WORD":       (2, 2, 0),
    "ROM_LOAD32_WORD_SWAP":  (2, 2, 1),
}

def parse_num(s):
    return int(s.strip(), 0)

class Load:
    def __init__(self, **kw):
        self.__dict__.update(dict(file=None, offset=0, length=0, crc=0,
                                  group=1, skip=0, reverse=0, kind="load",
                                  optional=0), **kw)

class Region:
    def __init__(self, kind, size, erase):
        self.kind, self.size, self.erase = kind, size, erase
        self.loads = []

games = {}
unknown_regions = set()
for m in re.finditer(r"^ROM_START\(\s*([a-z0-9_]+)\s*\)(.*?)^ROM_END", text, re.M | re.S):
    name, body = m.group(1), m.group(2)
    regions = []
    cur = None
    last_flags = (1, 0, 0)
    for st in expand_macros(body):
        st = re.sub(r"/\*.*?\*/", "", st).strip()
        if not st or st.startswith("//"):
            continue
        mm = re.match(r'ROM_REGION(?:16_BE)?\(\s*(0x[0-9a-fA-F]+)\s*,\s*"([^"]+)"\s*,\s*([^)]*)\)', st)
        if mm:
            rname = mm.group(2)
            if rname in SKIP_REGIONS:
                cur = None
                continue
            if rname == AUDIOBIOS:
                cur = Region("AB", parse_num(mm.group(1)), 0)
                regions.append(cur)
                continue
            if rname not in REGION_MAP:
                unknown_regions.add((name, rname))
                cur = None
                continue
            erase = 0xff if "ERASEFF" in mm.group(3) else 0x00
            cur = Region(REGION_MAP[rname], parse_num(mm.group(1)), erase)
            regions.append(cur)
            continue
        if cur is None:
            continue
        mm = re.match(r'(ROM_LOAD[0-9A-Z_]*)\(\s*"([^"]+)"\s*,\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*,\s*(?:BAD_DUMP\s+)?CRC\(([0-9a-fA-F]+)\)', st)
        if mm:
            macro = mm.group(1)
            if macro not in LOAD_KINDS:
                raise SystemExit("unhandled load macro %s in %s" % (macro, name))
            g, sk, rv = LOAD_KINDS[macro]
            cur.loads.append(Load(file=mm.group(2), offset=parse_num(mm.group(3)),
                                  length=parse_num(mm.group(4)), crc=int(mm.group(5), 16),
                                  group=g, skip=sk, reverse=rv))
            last_flags = (g, sk, rv)
            continue
        mm = re.match(r'ROM_LOAD[0-9A-Z_]*\(\s*"([^"]+)"\s*,\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*,\s*NO_DUMP\s*\)', st)
        if mm:
            # undumped ROM: mark with crc 0 and the nodump flag via group=0
            cur.loads.append(Load(file=mm.group(1), offset=parse_num(mm.group(2)),
                                  length=parse_num(mm.group(3)), crc=0, group=0))
            continue
        mm = re.match(r'ROM_CONTINUE\(\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*\)', st)
        if mm:
            g, sk, rv = last_flags
            cur.loads.append(Load(kind="continue", offset=parse_num(mm.group(1)),
                                  length=parse_num(mm.group(2)), group=g, skip=sk, reverse=rv))
            continue
        mm = re.match(r'ROM_IGNORE\(\s*([0-9xa-fA-F]+)\s*\)', st)
        if mm:
            cur.loads.append(Load(kind="ignore", length=parse_num(mm.group(1))))
            continue
        mm = re.match(r'ROM_RELOAD\(\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*\)', st)
        if mm:
            g, sk, rv = last_flags
            cur.loads.append(Load(kind="reload", offset=parse_num(mm.group(1)),
                                  length=parse_num(mm.group(2)), group=g,
                                  skip=sk, reverse=rv))
            continue
        mm = re.match(r'ROM_FILL\(\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*\)', st)
        if mm:
            cur.loads.append(Load(kind="fill", offset=parse_num(mm.group(1)),
                                  length=parse_num(mm.group(2)), crc=parse_num(mm.group(3))))
            continue
        mm = re.match(r'ROM_COPY\(\s*"([^"]+)"\s*,\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*,\s*([0-9xa-fA-F]+)\s*\)', st)
        if mm:
            src_kind = REGION_MAP.get(mm.group(1))
            if src_kind is None:
                raise SystemExit("ROM_COPY from unknown region %s in %s" % (mm.group(1), name))
            cur.loads.append(Load(kind="copy", file=src_kind, offset=parse_num(mm.group(3)),
                                  length=parse_num(mm.group(4)), crc=parse_num(mm.group(2))))
            continue
        if st.startswith(("ROM_REGION", "ROM_LOAD")):
            raise SystemExit("unparsed statement in %s: %s" % (name, st))
    # a cart audiocpu with no loads runs on the BIOS SM1 (dev boards:
    # dragonsh); ship that as the M ROM, optionally (it may live in
    # neogeo.zip rather than the set zip)
    mreg = next((r for r in regions if r.kind == "M"), None)
    abreg = next((r for r in regions if r.kind == "AB"), None)
    m_has_file = mreg is not None and any(l.kind == "load" for l in mreg.loads)
    if mreg is not None and not m_has_file and abreg is not None and abreg.loads:
        src = abreg.loads[0]
        mreg.size = src.length
        mreg.erase = 0x00
        mreg.loads.append(Load(file=src.file, offset=0, length=src.length,
                               crc=src.crc, optional=1))
    games[name] = [r for r in regions if r.kind != "AB"]

# ------------------------------------------------------------ GAME() lines --
def split_args(s):
    args, cur, depth, q = [], "", 0, False
    for ch in s:
        if ch == '"':
            q = not q
        if ch == '(' and not q:
            depth += 1
        if ch == ')' and not q:
            depth -= 1
        if ch == ',' and depth == 0 and not q:
            args.append(cur.strip()); cur = ""
        else:
            cur += ch
    args.append(cur.strip())
    return args

meta = {}
for m in re.finditer(r"^GAME\(\s*(.*?)\)\s*(?://.*|/\*.*)?$", text, re.M):
    a = split_args(m.group(1))
    year, name, parent, machine = a[0], a[1], a[2], a[3]
    manu, full = a[8].strip('"'), a[9].strip('"')
    meta[name] = dict(year=int(re.sub(r'[^0-9]', '', year)), parent=(None if parent in ("neogeo", "0") else parent),
                      machine=machine, manu=manu, full=full)

# ----------------------------------------------- machine -> cart slot slug --
slot = {}
fn = None
for line in text.splitlines():
    mm = re.match(r"^void [a-z0-9_]+_state::([a-z0-9_]+)\(machine_config &config\)", line)
    if mm:
        fn = mm.group(1)
        continue
    mm = re.search(r'cartslot_fixed\(config,\s*"([^"]+)"\)', line)
    if mm and fn:
        slot[fn] = mm.group(1)

# ------------------------------------------------------------------- emit ---
def esc(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')

kind_enum = {"P": "NC_REG_P", "S": "NC_REG_S", "M": "NC_REG_M", "MX": "NC_REG_MX",
             "V1": "NC_REG_V1", "V2": "NC_REG_V2", "C": "NC_REG_C"}
lkind_enum = {"load": "NC_LOAD", "continue": "NC_CONTINUE", "ignore": "NC_IGNORE",
              "fill": "NC_FILL", "copy": "NC_COPY", "reload": "NC_RELOAD"}

out = []
out.append("/* SPDX-License-Identifier: BSD-3-Clause\n"
           " * Generated by tools/gendb.py from MAME src/mame/snk/neogeo.cpp.\n"
           " * Do not edit by hand.  Data derived from the MAME project (BSD-3-Clause).\n"
           " */\n#include \"lithogen_internal.h\"\n\n")

emitted = []
for name, regions in sorted(games.items()):
    if name not in meta:            # BIOS-only ROM_START(neogeo) etc.
        continue
    md = meta[name]
    cart = slot.get(md["machine"])
    if cart is None:
        raise SystemExit("no cart slot for machine %s (game %s)" % (md["machine"], name))
    loads_rows = []
    reg_rows = []
    for r in regions:
        idx0 = None
        for ld in r.loads:
            row = '{ %s, %s, 0x%x, 0x%x, 0x%08x, %d, %d, %d, %d }' % (
                ('"%s"' % esc(ld.file)) if ld.kind in ("load",) else
                (('"%s"' % ld.file) if ld.kind == "copy" else "NULL"),
                lkind_enum[ld.kind], ld.offset, ld.length,
                ld.crc, ld.group, ld.skip, ld.reverse, ld.optional)
            loads_rows.append(row)
            if idx0 is None:
                idx0 = len(loads_rows) - 1
        reg_rows.append('{ %s, 0x%x, 0x%02x, %d, %d }' % (
            kind_enum[r.kind], r.size, r.erase,
            idx0 if idx0 is not None else 0, len(r.loads)))
    if loads_rows:
        out.append("static const nc_load loads_%s[] = {\n\t%s\n};\n" % (name, ",\n\t".join(loads_rows)))
    out.append("static const nc_region regs_%s[] = {\n\t%s\n};\n" % (name, ",\n\t".join(reg_rows)))
    emitted.append((name, md, cart, bool(loads_rows)))

out.append("\nconst nc_game nc_games[] = {\n")
for name, md, cart, has_loads in emitted:
    out.append('\t{ "%s", %s, "%s", "%s", %d, "%s", %d, regs_%s, %s },\n' % (
        name, ('"%s"' % md["parent"]) if md["parent"] else "NULL",
        esc(md["full"]), esc(md["manu"]), md["year"], cart,
        len(games[name]), name, ("loads_%s" % name) if has_loads else "NULL"))
out.append("};\n\nconst size_t nc_num_games = sizeof(nc_games) / sizeof(nc_games[0]);\n")

OUT.write_text("".join(out))
print("games emitted:", len(emitted))
if unknown_regions:
    print("NOTE: skipped unknown regions:", sorted(unknown_regions))
carts = sorted({c for _, _, c, _ in emitted})
print("cart slugs used (%d):" % len(carts))
print(" ", " ".join(carts))

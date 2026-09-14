#!/usr/bin/env python3
"""Dump header metadata from .neo files.

Usage:
    tools/dump_neo.py <dir-or-file> [more ...]

One tab-separated line per file:
    filename  year  genre  screenshot  ngh  name  manufacturer

Directories are scanned (non-recursively) for *.neo.  Output is sorted by
filename so runs diff cleanly.  Feed the output of a known-good set back
into tools/gen_meta.py to populate src/meta_db.c (genre/screenshot per
set for liblithogen's .neo writer).
"""
import hashlib
import os
import signal
import struct
import sys

# behave well in pipelines (dump_neo.py dir | head)
try:
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
except (AttributeError, ValueError):
    pass


def cstr(b):
    s = b.split(b"\0")[0].decode("ascii", errors="replace").rstrip()
    return s.replace("\t", " ")


def region_hashes(path, sizes):
    md5s = []
    with open(path, "rb") as f:
        f.seek(4096)
        for sz in sizes:
            md5s.append(hashlib.md5(f.read(sz)).hexdigest()[:12])
    return md5s


def dump_one(path):
    with open(path, "rb") as f:
        hdr = f.read(94)
    if len(hdr) < 94 or hdr[:4] != b"NEO\x01":
        print("skipping (not a .neo v1 file): %s" % path, file=sys.stderr)
        return None
    sizes = struct.unpack_from("<6I", hdr, 4)
    year, genre, shot, ngh = struct.unpack_from("<4I", hdr, 28)
    name = cstr(hdr[44:77])
    manu = cstr(hdr[77:94])
    return (os.path.basename(path), year, genre, shot, ngh, name, manu, sizes)


def collect(args):
    paths = []
    for a in args:
        if os.path.isdir(a):
            paths += [os.path.join(a, f) for f in os.listdir(a)
                      if f.lower().endswith(".neo")]
        else:
            paths.append(a)
    return sorted(paths, key=lambda p: os.path.basename(p).lower())


def main():
    args = sys.argv[1:]
    do_hash = "--hash" in args
    if do_hash:
        args.remove("--hash")
    if not args:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    paths = collect(args)
    rows = []
    for p in paths:
        r = dump_one(p)
        if r:
            rows.append((p,) + r)
    hdr = ("filename\tyear\tgenre\tscreenshot\tngh\tname\tmanu"
           "\tpsize\tssize\tmsize\tv1size\tv2size\tcsize")
    if do_hash:
        hdr += "\tpmd5\tsmd5\tmmd5\tv1md5\tv2md5\tcmd5"
    print(hdr)
    for row in rows:
        p, fn, year, genre, shot, ngh, name, manu, sz = row
        line = ("%s\t%u\t%u\t%u\t%03X\t%s\t%s\t%x\t%x\t%x\t%x\t%x\t%x" %
                ((fn, year, genre, shot, ngh, name, manu) + sz))
        if do_hash:
            line += "\t" + "\t".join(region_hashes(p, sz))
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())

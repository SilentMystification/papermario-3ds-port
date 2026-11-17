#!/usr/bin/env python3
# PSOSEEN: psoseen.py [--out FILE] [--selftest] [LOG ...]
# Merges the 'psoseen 0x...' lines of the bench run logs (nd_comb.c pso_seen_note: one line per distinct missed pso key)
# into 3ds/src/nd_pso_seen.c as ND_PSO_DESC lines. The ND_PSO_DESC lines already in FILE are kept (merge). Equal field
# texts count once. The lines are sorted by field text. Prints: psoseen keys <total> new <added>.
# --selftest: 3 log lines (2 equal) in a temp dir give keys 2 new 2, else exit 1.
import argparse
import os
import re
import sys
import tempfile
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "3ds", "src", "nd_pso_seen.c")
HEADER = (
    "/* nd_pso_seen.c: written by 3ds/tools/psoseen.py (PSOSEEN). Missed pso keys. */\n"
    '#include "nd_api.h"\n'
    '#pragma GCC diagnostic ignored "-Wunused-const-variable"\n'
)
LOG_RE = re.compile(r"psoseen (0x[0-9a-fA-F]+, .*?)\s*$")
DESC_RE = re.compile(r"^ND_PSO_DESC\(pso_seen_[0-9a-f]{8}, (.*)\);\s*$")


def log_texts(paths):
    out = []
    for path in paths:
        with open(path, "r", errors="replace") as f:
            for line in f:
                m = LOG_RE.search(line)
                if m:
                    out.append(m.group(1))
    return out


def existing_texts(path):
    if not os.path.exists(path):
        return []
    out = []
    with open(path, "r") as f:
        for line in f:
            m = DESC_RE.match(line.rstrip("\r\n"))
            if m:
                out.append(m.group(1))
    return out


def write_out(path, texts):
    lines = [HEADER]
    for t in texts:
        name = "pso_seen_%08x" % (zlib.crc32(t.encode("ascii")) & 0xFFFFFFFF)
        lines.append("ND_PSO_DESC(%s, %s);\n" % (name, t))
    with open(path, "w", newline="\n") as f:
        f.write("".join(lines))


def merge(logs, out):
    old = set(existing_texts(out))
    new_keys = set(log_texts(logs))
    texts = sorted(old | new_keys)
    added = len(new_keys - old)
    write_out(out, texts)
    return len(texts), added


def selftest():
    d = tempfile.mkdtemp(prefix="psoseen_")
    log = os.path.join(d, "run.log")
    a = "0x1, 0x2, 0x3, 0x0, 0x0, 0x4, 0x5, 0x6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 8"
    b = "0x9, 0x2, 0x3, 0x0, 0x0, 0x4, 0x5, 0x6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 8"
    with open(log, "w") as f:
        f.write("t 1 psoseen %s\n" % a)
        f.write("t 2 psoseen %s\n" % a)
        f.write("t 3 psoseen %s\n" % b)
    total, added = merge([log], os.path.join(d, "nd_pso_seen.c"))
    print("psoseen keys %d new %d" % (total, added))
    if (total, added) != (2, 2):
        print("psoseen selftest FAIL")
        return 1
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=OUT)
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("logs", nargs="*")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
    total, added = merge(args.logs, args.out)
    print("psoseen keys %d new %d" % (total, added))
    return 0


if __name__ == "__main__":
    sys.exit(main())

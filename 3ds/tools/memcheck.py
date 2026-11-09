#!/usr/bin/env python3
"""memcheck: does the ROM fit the console heap with this build's static size?

Reads the linker map (section sizes, __start__, __end__) and 3ds/tools/mem_budget.json, takes the code constants from the
source (pm_mem.c, pm_os.c, rom_remap.h), and computes the same sum the game does at boot (pm_mem.c __system_allocateHeaps,
pm_os.c pm_rom_init):
    image  = page_up(__end__) - __start__ + loader
    avail  = min(commit - image, fcram_cap)
    heap   = avail - linear - soc,  cap = heap - leave,  margin = cap - rom_need
Prints `memcheck: static <KB> margin <KB> device <dev> OK|OVER` and the 10 largest .data/.bss input sections.
Exit 0 on OK, 3 on OVER (a warning: callers decide), 2 on a missing map or budget.

Usage: python 3ds/tools/memcheck.py [--map build3ds/pm_3ds.map] [--device o3ds_xl] [--top 10] [--quiet]
"""
import argparse
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def page_up(x):
    return (x + 0xFFF) & ~0xFFF


def parse_map(path):
    """Returns (start, end, sections {name: size}, inputs [(size, name, obj)] of .data/.bss)."""
    start = end = None
    secs = {}
    inputs = []
    out = None
    pend = None  # an input section name whose address and size are on the next line
    started = False
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if not started:
                started = line.startswith("Linker script and memory map")
                continue
            if start is None:
                m = re.search(r"PROVIDE \(__start__ = (0x[0-9a-fA-F]+)\)", line)
                if m:
                    start = int(m.group(1), 16)
            m = re.match(r"\s+(0x[0-9a-fA-F]+)\s+__end__ = ABSOLUTE", line)
            if m:
                end = int(m.group(1), 16)
            m = re.match(r"^(\.[\w.]+)\s+0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)", line)
            if m:
                out = m.group(1)
                secs[out] = int(m.group(2), 16)
                pend = None
                continue
            if re.match(r"^\S", line):
                out = None
                continue
            if out not in (".data", ".bss"):
                continue
            m = re.match(r"^ (\.(?:data|bss)\S*|COMMON)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(\S+)", line)
            if m:
                inputs.append((int(m.group(3), 16), m.group(1), m.group(4)))
                pend = None
                continue
            m = re.match(r"^ (\.(?:data|bss)\S*|COMMON)\s*$", line)
            if m:
                pend = m.group(1)
                continue
            if pend:
                m = re.match(r"^\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(\S+)", line)
                if m:
                    inputs.append((int(m.group(2), 16), pend, m.group(3)))
                pend = None
    return start, end, secs, inputs


def code_values(root):
    """The constants the game uses, read from the source. Missing file or pattern: None (the budget copy is used)."""
    def grab(rel, pat, conv):
        try:
            with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as f:
                m = re.search(pat, f.read())
            return conv(m) if m else None
        except OSError:
            return None
    ours = grab("3ds/src/pm_mem.c", r"#define PM_OURS\s+\((\d+)u \* 1024u \* 1024u\)", lambda m: int(m.group(1)) * 1024)
    soc = grab("3ds/src/pm_mem.c", r"#define PM_SOC\s+(0x[0-9a-fA-F]+)u", lambda m: int(m.group(1), 16) >> 10)
    fc = grab("3ds/src/pm_mem.c", r"#define PM_FCRAM \((\d+)u \* 1024u \* 1024u\)", lambda m: int(m.group(1)) * 1024)
    leave = grab("3ds/src/pm_os.c", r"unsigned leave = (\d+)u \* 1024u;", lambda m: int(m.group(1)))
    need = grab("3ds/src/rom_remap.h", r"#define PM_ROM_PACKED (0x[0-9a-fA-F]+)u", lambda m: int(m.group(1), 16))
    v = {}
    if ours is not None and soc is not None:
        v["linear_kb"] = ours - soc
    if soc is not None:
        v["soc_kb"] = soc
    if fc is not None:
        v["fcram_cap_kb"] = fc
    if leave is not None:
        v["leave_kb"] = leave
    if need is not None:
        v["rom_need_b"] = need
    return v


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", default=os.path.join(ROOT, "build3ds", "pm_3ds.map"))
    ap.add_argument("--budget", default=os.path.join(ROOT, "3ds", "tools", "mem_budget.json"))
    ap.add_argument("--device", default="o3ds_xl")
    ap.add_argument("--top", type=int, default=10)
    ap.add_argument("--quiet", action="store_true", help="only the memcheck line")
    a = ap.parse_args()
    if not os.path.isfile(a.map):
        print("memcheck: no map %s" % a.map)
        return 2
    try:
        with open(a.budget, encoding="utf-8") as f:
            bud = json.load(f)
        dev = bud["devices"][a.device]
    except (OSError, ValueError, KeyError) as e:
        print("memcheck: budget %s device %s unreadable (%s)" % (a.budget, a.device, e))
        return 2
    code = {k: v["value"] for k, v in bud["code"].items() if not k.startswith("_")}
    src = code_values(ROOT)
    for k, v in src.items():
        if code.get(k) != v:
            print("memcheck: note %s is %s in the source, %s in mem_budget.json (the source value is used)" % (k, v, code.get(k)))
            code[k] = v

    start, end, secs, inputs = parse_map(a.map)
    if start is None or end is None:
        print("memcheck: no __start__ or __end__ in %s" % a.map)
        return 2
    static_b = page_up(end) - start
    commit = dev["commit_kb"]["value"] * 1024
    loader = dev["loader_kb"]["value"] * 1024
    avail = min(commit - static_b - loader, code["fcram_cap_kb"] * 1024)
    heap = avail - (code["linear_kb"] + code["soc_kb"]) * 1024
    cap = heap - code["leave_kb"] * 1024
    margin = cap - code["rom_need_b"]
    safety = bud["safety_kb"] * 1024
    verdict = "OK" if margin >= safety else "OVER"
    print("memcheck: static %d margin %d device %s %s" % (static_b >> 10, margin // 1024, a.device, verdict))
    if a.quiet:
        return 0 if verdict == "OK" else 3
    kinds = "".join("%s %s, " % (k, dev[k]["kind"]) for k in ("commit_kb", "loader_kb"))
    print("  sum KB: commit %d - static %d - loader %d - linear %d - soc %d - leave %d - rom_need %d = margin %d (safety %d; %s)" % (
        commit >> 10, static_b >> 10, loader >> 10, code["linear_kb"], code["soc_kb"], code["leave_kb"], code["rom_need_b"] >> 10,
        margin // 1024, safety >> 10, kinds.rstrip(", ")))
    print("  sections KB: " + " ".join("%s %d" % (n, secs[n] >> 10) for n in (".text", ".rodata", ".data", ".bss") if n in secs))
    inputs.sort(key=lambda t: -t[0])
    print("  largest .data/.bss:")
    for size, name, obj in inputs[:a.top]:
        print("    %7d KB  %-40s %s" % (size >> 10, name, os.path.basename(obj)))
    if verdict == "OVER":
        print("memcheck: OVER on %s: margin %d KB is below the %d KB safety. Below 0 KB the ROM does not fit in applet mode (\"Not enough memory for the ROM\")." % (
            a.device, margin // 1024, safety >> 10))
    return 0 if verdict == "OK" else 3


if __name__ == "__main__":
    sys.exit(main())

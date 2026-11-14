#!/usr/bin/env python3
"""ENTRYCAP: names the static lists that the device switch `entcap` logged (nd_walk.c entcap_rec).

Usage: entcap.py <log> [--elf 3ds/logs/builds/pm_3ds_<build>.elf]
Without --elf the newest 3ds/logs/builds/pm_3ds_*.elf is used.
Input lines: `entcap l <list> s <site> d <11 words>` (one per new walker entry description of a list without a v2 table)
and `entcap over` (the key table is full).
Output per entcap l line: <list symbol> <site function> <file:line> <the 11 words>.
Last line: `entcap.py lists <n> sites <m> over <count of entcap over lines>` (n = distinct list addresses, m = distinct sites).
Symbols come from arm-none-eabi-nm, names and lines from arm-none-eabi-addr2line (decode_dump.py:16-20 lookup).
The site is the nd_rec_dl call site: the return address minus 4 (the bl instruction).
"""
import bisect, glob, os, re, shutil, subprocess, sys

LINE = re.compile(r"entcap l ([0-9a-f]{8}) s ([0-9a-f]{8}) d((?: [0-9a-f]{8}){11})")
OVER = re.compile(r"entcap over")


IMG = "pm3ds-build:1"  # the build image (build.sh IMG): devkitARM in the container


def find_tool(name):
    for c in (shutil.which("arm-none-eabi-" + name), "/opt/devkitpro/devkitARM/bin/arm-none-eabi-" + name):
        if c and os.path.exists(c):
            return c
    return None


def tool_cmd(name, elf):
    """Host arm binutil, else the same binutil in the build image (the host has no devkitARM). Returns (argv prefix, elf arg)."""
    host = find_tool(name)
    if host:
        return [host], elf
    if not shutil.which("docker"):
        return None, None
    d = os.path.dirname(os.path.abspath(elf))
    return (["docker", "run", "--rm", "-v", d + ":/e:ro", "-w", "/e", IMG, "/opt/devkitpro/devkitARM/bin/arm-none-eabi-" + name],
            "/e/" + os.path.basename(elf))


def load_symbols(nm, elf):
    out = subprocess.run(nm + ["-n", elf], capture_output=True, text=True).stdout.split("\n")
    syms = []
    for ln in out:
        p = ln.split()
        if len(p) == 3:
            try:
                syms.append((int(p[0], 16), p[2]))
            except ValueError:
                pass
    syms.sort()
    return syms


def list_symbol(syms, addr):
    addrs = [a for a, _ in syms]
    k = bisect.bisect_right(addrs, addr) - 1
    if k < 0:
        return "??"
    a, name = syms[k]
    if a == addr:
        return name
    return "%s+0x%x" % (name, addr - a)


def site_name(addr2line, elf, site):
    out = subprocess.run(addr2line + ["-f", "-C", "-e", elf, "0x%x" % site], capture_output=True, text=True).stdout.split("\n")
    fn = out[0].strip() if out and out[0].strip() else "??"
    loc = out[1].strip() if len(out) > 1 and out[1].strip() else "??:0"
    return fn, loc


def main(a):
    if not a or a[0].startswith("-"):
        sys.exit(__doc__)
    log = a[0]
    elf = None
    if "--elf" in a:
        elf = a[a.index("--elf") + 1]
    if elf is None:
        c = sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "logs", "builds", "pm_3ds_*.elf")), key=os.path.getmtime)
        elf = c[-1] if c else None
    if not elf or not os.path.exists(elf):
        sys.exit("no ELF: pass --elf")
    nm, elf_nm = tool_cmd("nm", elf)
    addr2line, elf = tool_cmd("addr2line", elf)
    if not nm or not addr2line:
        sys.exit("arm-none-eabi-nm or arm-none-eabi-addr2line not found (host, or docker image %s)" % IMG)
    syms = load_symbols(nm, elf_nm)
    lists, sites, over = set(), set(), 0
    with open(log, "r", errors="replace") as f:
        for ln in f:
            if OVER.search(ln):
                over += 1
                continue
            m = LINE.search(ln)
            if not m:
                continue
            lst, site = int(m.group(1), 16), int(m.group(2), 16)
            words = m.group(3).split()
            lists.add(lst)
            sites.add(site)
            fn, loc = site_name(addr2line, elf, site - 4)
            print("%s %s %s %s" % (list_symbol(syms, lst), fn, loc, " ".join(words)))
    print("entcap.py lists %d sites %d over %d" % (len(lists), len(sites), over))


if __name__ == "__main__":
    main(sys.argv[1:])

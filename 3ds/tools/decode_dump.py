#!/usr/bin/env python3
"""Decode a Luma3DS arm11 exception dump (format v3.x, .dmp file).

Usage: decode_dump.py DUMP [--elf 3ds/logs/builds/pm_3ds_<build>.elf]
Without --elf the newest 3ds/logs/builds/pm_3ds_*.elf is used.
The dump holds no build number. pc and lr map to names only when the ELF is the one of the build that wrote the dump.
The tool checks this: it searches the code bytes of the dump in the ELF near pc and prints MATCH or NO MATCH.
Function names come from arm-none-eabi-addr2line (devkitARM). No devkitPro tool and no Luma3DS tool in the tree reads these dumps.
"""
import glob, os, shutil, struct, subprocess, sys

TYPES = {0: "FIQ", 1: "undefined instruction", 2: "prefetch abort", 3: "data abort"}
REGS = ["r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
        "cpsr", "dfsr", "ifsr", "far", "fpexc", "fpinst", "fpinst2"]


def find_tool():
    for c in (shutil.which("arm-none-eabi-addr2line"), "/opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line"):
        if c and os.path.exists(c):
            return c
    return None


def load_segments(path):
    d = open(path, "rb").read()
    if d[:4] != b"\x7fELF":
        sys.exit("not an ELF: " + path)
    phoff, = struct.unpack_from("<I", d, 28)
    phentsize, phnum = struct.unpack_from("<HH", d, 42)
    segs = []
    for i in range(phnum):
        t, off, va, pa, fsz, msz, fl, al = struct.unpack_from("<8I", d, phoff + i * phentsize)
        if t == 1 and fsz:
            segs.append((va, d[off:off + fsz]))
    return segs


def check_code(segs, code, pc):
    probe = code[8:40]
    if len(probe) < 16 or probe == b"\0" * len(probe):
        return "no code bytes in the dump"
    for va, data in segs:
        i = data.find(probe)
        while i >= 0:
            if abs(va + i - pc) < 0x200:
                return "MATCH: dump code found in the ELF at %08x" % (va + i)
            i = data.find(probe, i + 1)
    return "NO MATCH: the dump code is not in this ELF near pc. Names below are wrong. Use the ELF of the build that wrote the dump."


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    elf = None
    if "--elf" in a:
        k = a.index("--elf")
        elf = a[k + 1]
        del a[k:k + 2]
    if elf is None:
        c = sorted(glob.glob(os.path.join(os.path.dirname(__file__), "..", "logs", "builds", "pm_3ds_*.elf")), key=os.path.getmtime)
        elf = c[-1] if c else None
    d = open(a[0], "rb").read()
    if d[:8] != bytes.fromhex("dec0adde" "fecaadde"):
        sys.exit("not a Luma exception dump (magic)")
    major, minor = struct.unpack_from("<HH", d, 8)
    proc, typ, total, rsz, csz, ssz, asz = struct.unpack_from("<7I", d, 12)
    # Header is 40 bytes: magic(8) version(4) processor(4) type(4) total(4) reg(4) code(4) stack(4) extra(4)
    if (major, minor) != (3, 1):
        print("warning: untested dump version %d.%d" % (major, minor))
    o = 40
    regs = dict(zip(REGS, struct.unpack_from("<%dI" % (rsz // 4), d, o)))
    o += rsz
    code = d[o:o + csz]
    o += csz
    stack = d[o:o + ssz]
    o += ssz
    extra = d[o:o + asz]
    name = extra[:8].rstrip(b"\0").decode("ascii", "replace")
    tid = struct.unpack_from("<Q", extra, 8)[0] if len(extra) >= 16 else 0
    print("file %s: v%d.%d, processor %#x, %s, %d bytes, process %s, title %016x" % (a[0], major, minor, proc, TYPES.get(typ, typ), total, name, tid))
    for i in range(0, len(REGS), 4):
        print("  " + "  ".join("%-6s %08x" % (r, regs[r]) for r in REGS[i:i + 4] if r in regs))
    print("stack dump %d bytes, code dump %d bytes" % (len(stack), len(code)))
    if not elf:
        print("no ELF given and none in 3ds/logs/builds: no function names")
        return
    segs = load_segments(elf)
    print("ELF %s" % elf)
    print(check_code(segs, code, regs["pc"]))
    tool = find_tool()
    if not tool:
        print("arm-none-eabi-addr2line not found")
        return
    for r in ("pc", "lr"):
        out = subprocess.run([tool, "-f", "-C", "-e", elf, "0x%x" % regs[r]], capture_output=True, text=True).stdout.split("\n")
        print("%s %08x -> %s (%s)" % (r, regs[r], out[0], out[1] if len(out) > 1 else "?"))
    words = struct.unpack_from("<%dI" % (len(stack) // 4), stack) if stack else ()
    shown = 0
    for w in words:
        if 0x100000 <= w < 0x440000 and shown < 12:
            out = subprocess.run([tool, "-f", "-C", "-e", elf, "0x%x" % w], capture_output=True, text=True).stdout.split("\n")
            if out[0] != "??":
                print("  stack word %08x -> %s" % (w, out[0]))
                shown += 1


main()

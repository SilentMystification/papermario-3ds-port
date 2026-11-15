#!/usr/bin/env python3
"""Decode a Luma3DS arm11 exception dump (format v3.x, .dmp file), or read one [CRASH] block of a crash.txt.

Usage: decode_dump.py DUMP [--elf 3ds/logs/builds/pm_3ds_<build>.elf]
       decode_dump.py --crash FILE [--build N] [--elf ELF]
Without --elf the newest 3ds/logs/builds/pm_3ds_*.elf is used.
--crash reads the last [CRASH] abort block of FILE (or the block of build N) and names its pc, lr and stack words.
The crash.txt holds no code bytes, so the ELF must be the one of build N (--crash picks 3ds/logs/builds/pm_3ds_N*.elf).
The dump holds no build number. pc and lr map to names only when the ELF is the one of the build that wrote the dump.
The tool checks this: it searches the code bytes of the dump in the ELF near pc and prints MATCH or NO MATCH.
Function names come from arm-none-eabi-addr2line (devkitARM): the local tool, else one docker run of devkitpro/devkitarm.
No devkitPro tool and no Luma3DS tool in the tree reads these dumps.
"""
import glob, os, re, shutil, struct, subprocess, sys

TYPES = {0: "FIQ", 1: "undefined instruction", 2: "prefetch abort", 3: "data abort"}
REGS = ["r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
        "cpsr", "dfsr", "ifsr", "far", "fpexc", "fpinst", "fpinst2"]
HERE = os.path.dirname(os.path.abspath(__file__))


def find_tool():
    for c in (shutil.which("arm-none-eabi-addr2line"), "/opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line"):
        if c and os.path.exists(c):
            return c
    return None


def a2l(elf, addrs):
    """-> [(function, location)] for each address: the local tool, else ONE docker run (devkitpro/devkitarm). None if it failed."""
    ads = ["0x%x" % a for a in addrs]
    tool = find_tool()
    if tool:
        cmd = [tool, "-f", "-C", "-e", elf] + ads
    else:
        d, n = os.path.split(os.path.abspath(elf))
        # the image keeps the tool in /opt/devkitpro/devkitARM/bin, not on its PATH (checked 2026-11-15)
        cmd = ["docker", "run", "--rm", "-v", d + ":/e:ro", "devkitpro/devkitarm:latest",
               "/opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line", "-f", "-C", "-e", "/e/" + n] + ads
    r = subprocess.run(cmd, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    if not r.stdout.strip():
        err = r.stderr.strip().split("\n")[0] if r.stderr.strip() else "no output"
        print("addr2line failed: " + err)
        return None
    L = r.stdout.split("\n")
    return [(L[2 * i], L[2 * i + 1] if 2 * i + 1 < len(L) else "?") for i in range(len(addrs))]


def show_names(elf, pc, lr, words):
    """Print the names of pc, lr and the first 12 stack words that name a function (one a2l call)."""
    sw = []
    for w in words:
        if 0x100000 <= w < 0x440000 and w not in sw:
            sw.append(w)
    sw = sw[:1024]
    res = a2l(elf, [pc, lr] + sw)
    if res is None:
        return
    print("pc %08x -> %s (%s)" % (pc, res[0][0], res[0][1]))
    print("lr %08x -> %s (%s)" % (lr, res[1][0], res[1][1]))
    shown = 0
    for w, (fn, loc) in zip(sw, res[2:]):
        if fn != "??" and shown < 12:
            print("  stack word %08x -> %s" % (w, fn))
            shown += 1


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


def dump_main(a, elf):
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
    words = struct.unpack_from("<%dI" % (len(stack) // 4), stack) if stack else ()
    show_names(elf, regs["pc"], regs["lr"], words)


def crash_main(a, elf, build):
    """--crash FILE [--build N] [--elf ELF]: one [CRASH] abort block of a crash.txt."""
    lines = [l.rstrip("\r") for l in open(a[0], encoding="utf-8", errors="replace").read().split("\n")]
    is_abort = lambda l: "[CRASH] data abort" in l or "[CRASH] prefetch abort" in l
    starts = [i for i, l in enumerate(lines) if is_abort(l)]
    if build is not None:
        starts = [i for i in starts if re.search(r"build #%d\b" % build, lines[i])]
        if not starts:
            sys.exit("no abort block for build %d in %s" % (build, a[0]))
    if not starts:
        sys.exit("no [CRASH] abort block in " + a[0])
    i = starts[-1]
    j = i + 1
    while j < len(lines) and "[CRASH] exiting the process" not in lines[j] and not is_abort(lines[j]):
        j += 1
    block = lines[i:j]
    head = block[0]
    print(head)
    m = re.search(r"build #(\d+)", head)
    if not m:
        sys.exit("no build # in the block header")
    build = int(m.group(1))
    pc = int(re.search(r"\bpc=([0-9a-fA-F]{8})", head).group(1), 16)
    lr = int(re.search(r"\blr=([0-9a-fA-F]{8})", head).group(1), 16)
    words = []
    for l in block[1:]:
        if l.startswith("[CRASH] stack ") and not l.startswith("[CRASH] stack used"):
            words += [int(w, 16) for w in re.findall(r"\b[0-9a-fA-F]{8}\b", l[len("[CRASH] stack "):])]
    if elf is None:
        c = sorted(glob.glob(os.path.join(HERE, "..", "logs", "builds", "pm_3ds_%d*.elf" % build)), key=os.path.getmtime)
        if not c:
            print("no ELF for build %d: use --elf" % build)
            sys.exit(2)
        elf = c[-1]
    print("ELF %s" % elf)
    print("(crash.txt: no code bytes: the names are right only if the ELF is the build %d)" % build)
    show_names(elf, pc, lr, words)


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    crash = "--crash" in a
    if crash:
        a.remove("--crash")
    elf = build = None
    if "--elf" in a:
        k = a.index("--elf")
        elf = a[k + 1]
        del a[k:k + 2]
    if "--build" in a:
        k = a.index("--build")
        build = int(a[k + 1])
        del a[k:k + 2]
    if crash:
        return crash_main(a, elf, build)
    if elf is None:
        c = sorted(glob.glob(os.path.join(HERE, "..", "logs", "builds", "pm_3ds_*.elf")), key=os.path.getmtime)
        elf = c[-1] if c else None
    dump_main(a, elf)


main()

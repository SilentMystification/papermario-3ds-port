"""ELFGATE: the ELF half of the end proof (plan_endgame.md section 4).

Usage: python 3ds/tools/elfgate.py <elf>

Parses the ELF32 little-endian .symtab with struct only (no pip package), then counts:
  func     FUNC symbols whose base name (GCC clone suffix .constprop/.isra/.part/.cold stripped) matches a `func` line of
           3ds/tools/elfgate_patterns.txt.
  data     OBJECT symbols whose base name matches a `data` line of the same file.
  gfxnames OBJECT symbols whose name is a Gfx array name of the source scan (below).
  gfxscan_skip  Gfx N(name) definitions whose map namespace / NAME_PREFIX / NAME_SUFFIX could not be resolved.

Gfx source scan (repo root = two levels above this file): every src/**/*.c is a root; a src/**/*.inc.c that some scanned
file includes is not a root (it is scanned through its includer). Includes are resolved in assets/us first, then in the
including file's directory. Gfx names: `Gfx NAME[` and `Gfx N(name)[` (optional `static`). N(name) expands to
<NAMESPACE>_<NAME_PREFIX><name><NAME_SUFFIX>; each of the three macros comes from the file's own #define, else from the
includer. A macro defined with two different values in one file makes that N(name) unresolvable (skipped and counted).

Prints up to 20 matched names per class, then the line `elfgate func <n> data <m> gfxnames <k> gfxscan_skip <s>`.
Exits 0 on a normal run. A parse error raises (non-zero exit)."""
import mmap
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
CLONE = re.compile(r"\.(?:constprop|isra|part|cold)(?:\.\d+)?$")

GFX_DEF = re.compile(r"^\s*(?:static\s+)?Gfx\s+(\w+)\s*\[[^\]]*\]\s*=", re.M)
GFX_N_DEF = re.compile(r"^\s*(?:static\s+)?Gfx\s+N\((\w+)\)\s*\[[^\]]*\]\s*=", re.M)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+\.inc\.c)"', re.M)
NS_DEF = re.compile(r"^\s*#\s*define\s+NAMESPACE\s+(\w+)", re.M)
PREFIX_DEF = re.compile(r"^\s*#\s*define\s+NAME_PREFIX[ \t]*(\S*)", re.M)
SUFFIX_DEF = re.compile(r"^\s*#\s*define\s+NAME_SUFFIX[ \t]*(\S*)", re.M)

SHT_SYMTAB = 2
STT_OBJECT = 1
STT_FUNC = 2


def base_name(name):
    while True:
        stripped = CLONE.sub("", name)
        if stripped == name:
            return name
        name = stripped


def load_patterns():
    funcs, datas = [], []
    path = ROOT / "3ds" / "tools" / "elfgate_patterns.txt"
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        kind, _, regex = line.partition(" ")
        if kind == "func":
            funcs.append(re.compile(regex.strip()))
        elif kind == "data":
            datas.append(re.compile(regex.strip()))
        else:
            raise SystemExit(f"elfgate: bad pattern line: {line}")
    return funcs, datas


def cstr(mm, off):
    end = mm.find(b"\0", off)
    return mm[off:end].decode("latin-1")


def parse_elf(path):
    with open(path, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    if mm[:4] != b"\x7fELF" or mm[4] != 1 or mm[5] != 1:
        raise SystemExit(f"elfgate: {path} is not an ELF32 little-endian file")
    shoff = struct.unpack_from("<I", mm, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", mm, 0x2E)
    secs = []
    for i in range(shnum):
        # sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize
        secs.append(struct.unpack_from("<10I", mm, shoff + i * shentsize))
    shstr_off = secs[shstrndx][4]
    sec_names = [cstr(mm, shstr_off + s[0]) for s in secs]
    symtabs = [i for i, s in enumerate(secs) if s[1] == SHT_SYMTAB]
    if len(symtabs) != 1:
        raise SystemExit(f"elfgate: expected one .symtab, found {len(symtabs)}")
    st = secs[symtabs[0]]
    str_off = secs[st[6]][4]
    syms = []
    count = st[5] // 16
    for i in range(count):
        st_name, st_value, st_size, st_info, _other, st_shndx = struct.unpack_from(
            "<IIIBBH", mm, st[4] + i * 16)
        if st_name == 0:
            continue
        typ = st_info & 0xF
        if 0 < st_shndx < 0xFF00:
            sec = sec_names[st_shndx]
        else:
            sec = {0: "UNDEF", 0xFFF1: "ABS"}.get(st_shndx, f"shndx{st_shndx}")
        syms.append((cstr(mm, str_off + st_name), st_value, st_size, typ, sec))
    return syms, count


def single_value(regex, text):
    """Unique value of a #define macro in one file: a value, '' for a bare define, None for none or several."""
    vals = set(m.group(1) for m in regex.finditer(text))
    if len(vals) == 1:
        return vals.pop()
    return None if len(vals) > 1 else "?none?"


def resolve(own, inherited):
    if own is None:
        return None
    return inherited if own == "?none?" else own


def gfx_source_scan():
    src = ROOT / "src"
    assets = ROOT / "assets" / "us"
    names = set()
    stats = {"skip": 0, "missing_inc": 0, "files": 0}
    visited = set()
    included = set()

    def find_inc(inc, includer):
        p = assets / inc
        if p.exists():
            return p
        p = includer.parent / inc
        return p if p.exists() else None

    def scan(path, ctx, depth=0):
        key = (path, ctx)
        if key in visited:
            return
        visited.add(key)
        stats["files"] += 1
        text = path.read_text(encoding="utf-8", errors="replace")
        ns = resolve(single_value(NS_DEF, text), ctx[0])
        pre = resolve(single_value(PREFIX_DEF, text), ctx[1])
        suf = resolve(single_value(SUFFIX_DEF, text), ctx[2])
        for name in GFX_DEF.findall(text):
            names.add(name)
        for name in GFX_N_DEF.findall(text):
            if ns is None or pre is None or suf is None:
                stats["skip"] += 1
            else:
                names.add(f"{ns}_{pre}{name}{suf}")
        for inc in INCLUDE.findall(text):
            p = find_inc(inc, path)
            if p is None:
                stats["missing_inc"] += 1
                continue
            scan(p, (ns, pre, suf), depth + 1)

    src_c = sorted(src.glob("**/*.c"))
    for p in src_c:
        for inc in INCLUDE.findall(p.read_text(encoding="utf-8", errors="replace")):
            q = find_inc(inc, p)
            if q is not None:
                included.add(q.resolve())
    for p in src_c:
        if p.name.endswith(".inc.c") and p.resolve() in included:
            continue
        scan(p, (None, None, None))
    return names, stats


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: elfgate.py <elf>")
    elf = sys.argv[1]
    funcs, datas = load_patterns()
    syms, total = parse_elf(elf)
    gfx, stats = gfx_source_scan()

    matched = {"func": [], "data": [], "gfxnames": []}
    for name, value, size, typ, sec in syms:
        b = base_name(name)
        if typ == STT_FUNC and any(r.search(b) for r in funcs):
            matched["func"].append((name, value, size, sec))
        if typ == STT_OBJECT and any(r.search(b) for r in datas):
            matched["data"].append((name, value, size, sec))
        if typ == STT_OBJECT and name in gfx:
            matched["gfxnames"].append((name, value, size, sec))

    for cls in ("func", "data", "gfxnames"):
        for name, value, size, sec in matched[cls][:20]:
            print(f"{cls} {name} value=0x{value:08x} size={size} section={sec}")
    print(f"elfgate symbols {total} gfxset {len(gfx)} gfxscan_files {stats['files']} "
          f"gfxscan_missing_inc {stats['missing_inc']}")
    print(f"elfgate func {len(matched['func'])} data {len(matched['data'])} "
          f"gfxnames {len(matched['gfxnames'])} gfxscan_skip {stats['skip']}")


if __name__ == "__main__":
    main()

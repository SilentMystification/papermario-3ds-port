"""ELFGATE: the ELF half of the end proof (plan_endgame.md section 4).

Usage: python 3ds/tools/elfgate.py <elf>

Parses the ELF32 little-endian .symtab with struct only (no pip package), then counts:
  func     FUNC symbols whose base name (GCC clone suffix .constprop/.isra/.part/.cold stripped) matches a `func` line of
           3ds/tools/elfgate_patterns.txt.
  data     OBJECT symbols whose base name matches a `data` line of the same file.
  gfxnames OBJECT symbols whose name is a Gfx array name of the source scan (below).
  gfxscan_skip  Gfx N(name) definitions whose map namespace / NAME_PREFIX / NAME_SUFFIX could not be resolved.

Gfx source scan (ORACLE8 correction 1): every *.c file under <repo>/src and <asset root>/assets/us, each file read on its own
(no include following). The asset root is the repo when it has assets/us, else the main folder (git rev-parse
--git-common-dir + "/.."), as 3ds/build.sh:15-17. Gfx names: `Gfx NAME[` (plain) and `Gfx N(name)[`. N(name) expands to
NAMESPACE_ + NAME_PREFIX + name + NAME_SUFFIX. NAMESPACE is the file's first #define, else the first existing `#include "..."`
target (one level, include order) that has one; none is counted in gfxscan_skip. NAME_PREFIX / NAME_SUFFIX are the last
define above the definition in the same file (empty when none).

Prints up to 20 matched names per class, then `elfgate gfxset <n> gfxscan_files <f>` and the line
`elfgate func <n> data <m> gfxnames <k> gfxscan_skip <s>`. Exits 0 on a normal run. A parse error raises (non-zero exit)."""
import mmap
import pathlib
import re
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
CLONE = re.compile(r"\.(?:constprop|isra|part|cold)(?:\.\d+)?$")

GFX_DEF = re.compile(r"\bGfx\s+(?:N\((\w+)\)|(\w+))\s*\[")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
NS_DEF = re.compile(r"^\s*#\s*define\s+NAMESPACE\s+(\w+)", re.M)
NAME_DEF = re.compile(r"^[ \t]*#[ \t]*define[ \t]+NAME_(PREFIX|SUFFIX)[ \t]*(\w*)", re.M)

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


def asset_root():
    """The repo when it has assets/us, else the main folder (build.sh:15-17: git rev-parse --git-common-dir + "/..")."""
    if (ROOT / "assets" / "us").is_dir():
        return ROOT
    out = subprocess.run(["git", "rev-parse", "--git-common-dir"], cwd=ROOT, capture_output=True, text=True, check=True)
    return (ROOT / out.stdout.strip()).resolve().parent


def gfx_source_scan():
    roots = [ROOT / "src", asset_root() / "assets" / "us"]
    files = []
    for r in roots:
        files += sorted(r.glob("**/*.c"))
    names = set()
    stats = {"skip": 0, "files": len(files)}

    def read(p):
        return p.read_text(encoding="utf-8", errors="replace")

    def file_namespace(p, text):
        m = NS_DEF.search(text)
        if m:
            return m.group(1)
        for inc in INCLUDE.findall(text):
            q = p.parent / inc
            if q.is_file():
                m = NS_DEF.search(read(q))
                if m:
                    return m.group(1)
        return None

    for p in files:
        text = read(p)
        defs = list(NAME_DEF.finditer(text))
        ns = None
        ns_done = False
        for m in GFX_DEF.finditer(text):
            if m.group(1) is None:
                names.add(m.group(2))
                continue
            if not ns_done:
                ns = file_namespace(p, text)
                ns_done = True
            if ns is None:
                stats["skip"] += 1
                continue
            pre = suf = ""
            for d in defs:
                if d.start() >= m.start():
                    break
                if d.group(1) == "PREFIX":
                    pre = d.group(2)
                else:
                    suf = d.group(2)
            names.add(f"{ns}_{pre}{m.group(1)}{suf}")
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
    print(f"elfgate gfxset {len(gfx)} gfxscan_files {stats['files']}")
    print(f"elfgate symbols {total}")
    print(f"elfgate func {len(matched['func'])} data {len(matched['data'])} "
          f"gfxnames {len(matched['gfxnames'])} gfxscan_skip {stats['skip']}")


if __name__ == "__main__":
    main()

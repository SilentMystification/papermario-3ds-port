"""S7CONV: the symbol scan of the gfx table tool (gen_gfx_tab.sh, row S7CONV design note 2).
Usage: gen_gfx_syms.py <out gconv_in.c> <gfx source .c> ...
Writes the shared file <out gconv_in.c> (it includes the shared sources: effects, model and GSET twins) with the
array gconv_sym_in[]. Each own-TU source k (any other source) gets its own file gconv_tu<k>.c next to <out gconv_in.c>
(it includes its source, nd_gconv.h and its entries as gconv_sym_tu<k>[]). Every array sits in the section gconv_sym
(GCONV_SEC of 3ds/tools/nd_gconv.h). Prints the path of each gconv_tu<k>.c on stdout, one per line.
Symbols: every non-static `Gfx NAME[] =`, `Vtx NAME[..] =` and `Mtx NAME =`, `Lights1 NAME =` of the sources and their `#include "*.inc.c"`
files (under assets/us, src or next to the includer), named `N(x)` as NAMESPACE_x; and every INCLUDE_IMG/PAL/RAW NAME (its
end from the NAME_gend label of 3ds/tools/gconv_inc/include_asset.h). Run from the repo root."""
import re
import sys
import pathlib

DEF = re.compile(r"^(Gfx|Vtx|Vec3s)\s+(?:N\((\w+)\)|(\w+))\s*\[[^\]]*\]\s*=", re.M)
JP = re.compile(r"^#if VERSION_(?:JP|PAL)\b.*?^#(?:else|endif)\b", re.M | re.S)  # FXENT3: the JP branch is not in the ELF; LIT1: nor the PAL one (US build)
MTX = re.compile(r"^Mtx\s+(?:N\((\w+)\)|(\w+))\s*(\[[^\]]*\])?\s*=", re.M)
LIT = re.compile(r"^Lights1\s+(?:N\((\w+)\)|(\w+))\s*=", re.M)  # LIT1
INC = re.compile(r'^\s*#include\s+"([^"]+\.inc\.c)"', re.M)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
IMG = re.compile(r"INCLUDE_(?:IMG|PAL|RAW)\(\s*\"[^\"]*\"\s*,\s*(\w+)\s*\)")
NS_DEF = re.compile(r"^\s*#\s*define\s+NAMESPACE\s+(\w+)", re.M)
NAME_DEF = re.compile(r"^[ \t]*#[ \t]*define[ \t]+NAME_(PREFIX|SUFFIX)[ \t]*(\w*)", re.M)


def resolve(inc, path):
    # GCONVW: assets/us first, then src, then the directory of the includer
    for p in (pathlib.Path("assets/us") / inc, pathlib.Path("src") / inc, path.parent / inc):
        if p.exists():
            return p
    return None


def file_ns(path, text, parent_ns):
    # GCONVW: the file's own NAMESPACE, else the one of a direct include, else the one of the includer
    m = NS_DEF.search(text)
    if m:
        return m.group(1)
    for inc in INCLUDE.findall(text):
        q = resolve(inc, path)
        if q is not None:
            m = NS_DEF.search(q.read_text(encoding="utf-8", errors="replace"))
            if m:
                return m.group(1)
    return parent_ns


def name_of(path, ns, nx, plain, pre_bad):
    if plain:
        return plain
    if pre_bad:
        sys.exit(f"gen_gfx_syms: {path}: N({nx}) in a file with NAME_PREFIX or NAME_SUFFIX")
    if ns is None:
        sys.exit(f"gen_gfx_syms: {path}: N({nx}) without NAMESPACE")
    return f"{ns}_{nx}"


def scan(path, out, moved=False, parent_ns=None):
    text = path.read_text(encoding="utf-8", errors="replace")
    text = JP.sub("", text)  # FXENT3
    ns = file_ns(path, text, parent_ns)
    pre_bad = any(d.group(2) for d in NAME_DEF.finditer(text))
    for m in DEF.finditer(text):
        out.append((m.group(1), name_of(path, ns, m.group(2), m.group(3), pre_bad), moved))
    for m in MTX.finditer(text):
        out.append(("Mtx", name_of(path, ns, m.group(1), m.group(2), pre_bad), moved))
    for m in LIT.finditer(text):
        out.append(("Lit", name_of(path, ns, m.group(1), m.group(2), pre_bad), moved))
    for name in IMG.findall(text):
        out.append(("Img", name, moved))
    for inc in INC.findall(text):
        p = resolve(inc, path)
        if p is None:
            sys.exit(f"gen_gfx_syms: include {inc} of {path} not found")
        scan(p, out, moved, ns)


TWIN = "3ds/tools/gfxin/"
SHARED = ("src/effects/", "src/entity/model/")  # GSETB: a twin is shared when its device source is


def scan_source(s, syms):
    # GSET: a twin holds the moved includes of its device source; its first line names the source
    if not s.startswith(TWIN):
        scan(pathlib.Path(s), syms)
        return
    twin = pathlib.Path(s)
    first = twin.read_text(encoding="utf-8", errors="replace").splitlines()[0]
    m = re.match(r'#include\s+"([^"]+)"', first)
    if not m:
        sys.exit(f"gen_gfx_syms: twin {s} first line is not an #include")
    src = pathlib.Path(m.group(1))
    scan(src, syms, False)
    # GSETW: the twin's N(x) names take the NAMESPACE of its device source (the twin has none of its own)
    scan(twin, syms, True, file_ns(src, src.read_text(encoding="utf-8", errors="replace"), None))


def entry_lines(entries):
    lines = []
    for k, n, mv in entries:
        tail = ", 1 }," if mv else " },"
        if k == "Img":
            lines.append(f'    {{ "{n}", GS_IMG, (const void*){n}, (const void*){n}_gend{tail}')
        else:
            gk = {"Gfx": "GS_GFX", "Vtx": "GS_VTX", "Mtx": "GS_MTX", "Lit": "GS_LIT", "Vec3s": "GS_V3S"}[k]
            if k in ("Mtx", "Lit"):
                lines.append(f'    {{ "{n}", {gk}, (const void*)&{n}, (const void*)((const char*)&{n} + sizeof {n}){tail}')
            else:
                lines.append(f'    {{ "{n}", {gk}, (const void*){n}, (const void*)((const char*){n} + sizeof {n}){tail}')
    return lines


def write_file(path, srcs, entries, tag):
    lines = ["/* Generated by 3ds/tools/gen_gfx_syms.py (GCONVW): the inputs of the gfx table tool and their symbol map. */"]
    lines += [f'#include "{s}"' for s in srcs]
    lines += ['#include "nd_gconv.h"']
    for k, n, mv in entries:
        if k == "Img":
            lines.append(f"extern unsigned char {n}_gend[];")
    if entries:
        lines.append(f"static const GSym gconv_sym_{tag}[] GCONV_SEC = {{")
        lines += entry_lines(entries)
        lines.append("};")
    pathlib.Path(path).write_text("\n".join(lines) + "\n")


def main():
    out_c, srcs = sys.argv[1], list(dict.fromkeys(sys.argv[2:]))
    out_dir = pathlib.Path(out_c).parent
    seen = set()
    shared_srcs, shared_entries = [], []
    tu_files = []
    for k, s in enumerate(srcs):
        syms = []
        scan_source(s, syms)
        mine = []
        for kind, n, mv in syms:
            if n not in seen:
                seen.add(n)
                mine.append((kind, n, mv))
        if (s[len(TWIN):] if s.startswith(TWIN) else s).startswith(SHARED):
            shared_srcs.append(s)
            shared_entries += mine
        else:
            path = out_dir / f"gconv_tu{k}.c"
            write_file(path, [s], mine, f"tu{k}")
            tu_files.append(str(path))
    write_file(out_c, shared_srcs, shared_entries, "in")
    print(f"gen_gfx_syms: {len(srcs)} sources {len(seen)} symbols", file=sys.stderr)
    for f in tu_files:
        print(f)


main()

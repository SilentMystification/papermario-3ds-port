#!/usr/bin/env python3
# NPSO3: gen_pso_tab.py <out.c> <src dir>...
# The build-time pipeline table. Scans every .c file under the dirs for ND_PSO_DESC( calls (file name order, then line),
# decides each description once on the host (gen_pso_main.c, linked with nd_decide.c, nd_tmpl.c, pm_comb.c and the
# descriptions as pso_descs.c) and writes <out.c>: the NdPsoOut bytes of each id and nd_pso(id). A duplicate name is an
# error (both file:line). Run from the repo root (or the host image cwd with the same layout). Comments and literals are masked
# for the scan, so a call named in a comment or a string is not a description.
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FLAGS = ["-DF3DEX_GBI_2", "-D_LANGUAGE_C", "-DTARGET_3DS", "-DVERSION_US", "-D_MIPS_SZLONG=32",
         "-I", "include", "-I", "ver/us/build/include", "-I", "3ds/include", "-I", "3ds/stub_include", "-I", "src"]
PAT = re.compile(r"\bND_PSO_DESC\s*\(")
NAME = re.compile(r"[A-Za-z_]\w*\Z")


def blank(s):
    return "".join(ch if ch == "\n" else " " for ch in s)


def mask(text):
    """The text with comments and string or char literals blanked (newlines kept), so offsets and lines match."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(blank(text[i:j]))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(blank(text[i:j]))
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] not in (c, "\n"):
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(blank(text[i:j]))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def calls(path):
    """[(name, call text, line)] of the ND_PSO_DESC( calls of one file, in order."""
    with open(path, encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    if "ND_PSO_DESC" not in text:
        return []
    m = mask(text)
    res = []
    for hit in PAT.finditer(m):
        line = m.count("\n", 0, hit.start()) + 1
        ls = m.rfind("\n", 0, hit.start()) + 1
        if m[ls:hit.start()].lstrip().startswith("#"):
            continue  # a #define line, not a call
        i, depth, comma = hit.end(), 1, None
        while i < len(m) and depth:
            ch = m[i]
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            elif ch == "," and depth == 1 and comma is None:
                comma = i
            i += 1
        if depth:
            raise SystemExit(f"{path}:{line}: ND_PSO_DESC( is not closed")
        name = text[hit.end():comma if comma is not None else i - 1].strip()
        if not NAME.match(name):
            raise SystemExit(f"{path}:{line}: ND_PSO_DESC name is not an identifier: {name!r}")
        res.append((name, text[hit.start():i], line))
    return res


def run(cmd):
    r = subprocess.run(cmd)
    if r.returncode:
        raise SystemExit(f"gen_pso_tab: failed ({r.returncode}): {' '.join(cmd)}")


def main(argv):
    if len(argv) < 3:
        print("usage: gen_pso_tab.py <out.c> <src dir>...", file=sys.stderr)
        return 2
    out, dirs = argv[1], argv[2:]
    files = set()
    for d in dirs:
        for root, _, names in os.walk(d):
            for nm in names:
                if nm.endswith(".c"):
                    files.add(os.path.join(root, nm).replace("\\", "/"))
    seen, entries = {}, []
    for f in sorted(files):
        for name, call, line in calls(f):
            where = f"{f}:{line}"
            if name in seen:
                raise SystemExit(f"gen_pso_tab: duplicate ND_PSO_DESC {name}: {seen[name]} and {where}")
            seen[name] = where
            entries.append((name, call))
    tmp = tempfile.mkdtemp(prefix="psotab_")
    try:
        descs_c = os.path.join(tmp, "pso_descs.c")
        with open(descs_c, "w", encoding="utf-8") as fh:
            fh.write('#include "nd_api.h"\n')
            for name, call in entries:
                fh.write(call + ";\n")
            ptrs = ", ".join(f"&{n}" for n, _ in entries) or "0"
            names = ", ".join(f'"{n}"' for n, _ in entries) or "0"
            fh.write(f"const NdPsoDesc* const pso_descs[] = {{ {ptrs}" + (", 0" if entries else "") + " };\n")
            fh.write(f"const char* const pso_names[] = {{ {names}" + (", 0" if entries else "") + " };\n")
        pmo = os.path.join(tmp, "pm_comb.o")
        run(["gcc", "-m32", "-w", *FLAGS, "-c", "-o", pmo, "3ds/src/pm_comb.c"])
        gen = os.path.join(tmp, "psogen")
        run(["gcc", "-m32", "-Wall", "-Wextra", "-Werror", *FLAGS, "-o", gen, os.path.join(HERE, "gen_pso_main.c"),
             "3ds/src/nd_decide.c", "3ds/src/nd_tmpl.c", descs_c, pmo, "-lm"])
        run([gen, out])
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"gen_pso_tab: {len(entries)} descs -> {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

#!/usr/bin/env python3
"""nd_size_check.py <nm.txt> <objdump.txt>: sum of nd_emit and its callees (G5) and the instruction count of the DRAW_IDX path.
Limits: 8192 bytes and 120 instructions (plan_native_system 9.5 [U target]). The DRAW_IDX path is the text between the labels
nd_draw_idx_start and nd_draw_idx_end when both exist, else the whole of nd_emit. Exit 0 pass, 1 over the limit, 2 bad input."""
import re, sys

BYTES_MAX, INSN_MAX = 8192, 120
NM = re.compile(r"^[0-9a-f]+ ([0-9a-f]+) [tT] (\S+)$")
FUNC = re.compile(r"^[0-9a-f]+ <(\S+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\t")
CALL = re.compile(r"\b(?:bl|blx|b)\s+[0-9a-f]+ <([^>+]+)")
RELOC = re.compile(r"^\s*[0-9a-f]+: R_ARM_\w+\s+(\S+)$")  # objdump -dr: call targets in a .o are relocations


def main(nm_path, od_path):
    sizes, funcs, order = {}, {}, []
    for ln in open(nm_path, encoding="ascii"):
        m = NM.match(ln.strip())
        if m:
            sizes[m.group(2)] = int(m.group(1), 16)
    cur = None
    for ln in open(od_path, encoding="ascii", errors="replace"):
        ln = ln.rstrip("\n")
        m = FUNC.match(ln)
        if m:
            cur = m.group(1); funcs[cur] = []; order.append(cur); continue
        if cur is not None and INSN.match(ln):
            funcs[cur].append(ln)
        elif cur is not None and RELOC.match(ln):
            funcs[cur].append("reloc " + RELOC.match(ln).group(1))
    if "nd_emit" not in funcs:
        print("FAIL nd_emit not in the object")
        return 2
    seen, todo = set(), ["nd_emit"]
    while todo:
        f = todo.pop()
        if f in seen or f not in funcs:
            continue
        seen.add(f)
        for ln in funcs[f]:
            for c in CALL.findall(ln):
                todo.append(c)
            if ln.startswith("reloc "):
                todo.append(ln[6:])
    total = sum(sizes.get(f, 0) for f in seen)
    body = funcs["nd_emit"]
    labels = [i for i, ln in enumerate(body) if "<nd_draw_idx_start>" in ln or "<nd_draw_idx_end>" in ln]
    path = body
    if len(labels) == 2:
        path = body[labels[0]:labels[1]]
    insn = sum(1 for ln in path if not ln.startswith("reloc "))
    ok_b, ok_i = total <= BYTES_MAX, insn <= INSN_MAX
    print("NDSIZE functions %s" % ",".join(sorted(seen)))
    print("NDSIZE bytes %d limit %d %s" % (total, BYTES_MAX, "ok" if ok_b else "OVER"))
    print("NDSIZE draw_idx insn %d limit %d %s" % (insn, INSN_MAX, "ok" if ok_i else "OVER"))
    return 0 if ok_b and ok_i else 1


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1], sys.argv[2]))

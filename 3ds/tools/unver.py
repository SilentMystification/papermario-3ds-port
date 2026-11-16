#!/usr/bin/env python3
"""unver.py: DEADSRC. Resolve the version conditionals of the given C files for the device build (VERSION_US 1; VERSION_PAL, VERSION_JP,
VERSION_IQUE 0). A conditional chain (#if/#ifdef/#ifndef .. #elif .. #else .. #endif) is resolved only when each of its conditions uses
only these four names, `defined(...)` of them, !, &&, || and brackets: its directive lines go, the lines of the taken branch stay, the
other branches go. Any other chain stays as it is (its inner chains are still resolved).
Usage: unver.py [--dry] <file>...  Prints `unver file <path> chains <c> lines <removed>` per file, then `unver total chains <c> lines <r>`."""
import re
import sys

VERSIONS = {"VERSION_US": 1, "VERSION_PAL": 0, "VERSION_JP": 0, "VERSION_IQUE": 0}
EVAL_RULE = True  # DEADSRC seed: the gate script runs a copy with False: VERSION_PAL counts as 1, so src/starpoint.c loses its US lists
DIR = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def cond(word, rest):
    """1 or 0 for a condition of the four names, None for any other condition."""
    vals = dict(VERSIONS)
    if not EVAL_RULE:
        vals["VERSION_PAL"] = 1
    r = re.sub(r"//.*$|/\*.*?\*/", "", rest).strip()
    if word in ("ifdef", "ifndef"):
        if r not in vals:
            return None
        v = 1 if vals[r] else 0
        return v if word == "ifdef" else 1 - v
    names = set(re.findall(r"[A-Za-z_]\w*", r)) - {"defined"}
    if not names or not names <= set(vals) or re.search(r"[^\w\s()!&|]", r):
        return None
    e = re.sub(r"defined\s*\(\s*(\w+)\s*\)", lambda m: "1" if vals[m.group(1)] else "0", r)
    e = re.sub(r"defined\s+(\w+)", lambda m: "1" if vals[m.group(1)] else "0", e)
    for k, v in vals.items():
        e = re.sub(r"\b%s\b" % k, str(v), e)
    e = e.replace("&&", " and ").replace("||", " or ")
    e = re.sub(r"!(?!=)", " not ", e)
    return 1 if eval(e) else 0


def resolve(lines):
    # pass 1: chain id of each directive line, and whether every condition of the chain is a version condition
    chain_of, ok, stack, nid = {}, [], [], 0
    for i, ln in enumerate(lines):
        m = DIR.match(ln)
        if not m:
            continue
        w, rest = m.group(1), m.group(2)
        if w in ("if", "ifdef", "ifndef"):
            stack.append(nid)
            ok.append(cond(w, rest) is not None)
            nid += 1
        if not stack:
            continue
        c = stack[-1]
        chain_of[i] = c
        if w == "elif" and cond("if", rest) is None:
            ok[c] = False
        if w == "endif":
            stack.pop()
    # pass 2: emit
    out, st, chains = [], [], 0
    for i, ln in enumerate(lines):
        live = all(f["on"] for f in st)
        m = DIR.match(ln)
        c = chain_of.get(i)
        if m and c is not None:
            w, rest = m.group(1), m.group(2)
            if w in ("if", "ifdef", "ifndef"):
                if ok[c]:
                    v = cond(w, rest)
                    st.append({"res": True, "on": bool(v), "taken": bool(v)})
                    chains += live
                    continue
                st.append({"res": False, "on": True, "taken": True})
            elif st[-1]["res"]:
                f = st[-1]
                if w == "elif":
                    v = cond("if", rest)
                    f["on"] = (not f["taken"]) and bool(v)
                    f["taken"] = f["taken"] or bool(v)
                elif w == "else":
                    f["on"] = not f["taken"]
                    f["taken"] = True
                else:
                    st.pop()
                continue
            elif w == "endif":
                st.pop()
            if live:
                out.append(ln)
            continue
        if live:
            out.append(ln)
    return out, chains


def main(argv):
    dry = "--dry" in argv
    tc = tr = 0
    for f in [a for a in argv if a != "--dry"]:
        with open(f, encoding="utf-8", newline="") as fh:
            text = fh.read()
        lines = text.splitlines(keepends=True)
        out, chains = resolve(lines)
        removed = len(lines) - len(out)
        print(f"unver file {f} chains {chains} lines {removed}")
        tc += chains
        tr += removed
        if not dry and removed:
            with open(f, "w", encoding="utf-8", newline="") as fh:
                fh.write("".join(out))
    print(f"unver total chains {tc} lines {tr}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

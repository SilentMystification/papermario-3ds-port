#!/usr/bin/env python3
"""seedprobe.py (SEEDTOOL): one verdict line per seed. A seed is one source edit that changes a value in a draw function;
one build, one run and one cmp check all seeds of a call together. Stored base only (bench.py base, no run for the base).

  python 3ds/tools/seedprobe.py <bench> <seed> [<seed> ...] [--write <seeds file>]

  seed = <file>:<line>@<frame>|<old text>|<new text>
    <file>: path from the worktree root. <line>: 1-based line; <old text> must occur exactly once on it, else bad-spec.
    <frame>: the drec frame the seed must change, or `-` for a whole-window bench.
  Run from the worktree root with PM_ROW and PM_LANE set. Output: one verdict line per seed:
    seed <file>:<line> live (frame <f> differs) | dead (frame <f> equal)     (@-: live (window differs) | dead (window equal))
  --write <seeds file>: for each verdict, the seeds-file line whose first field is the file stem and whose fourth field is
    the line number gets its last field set to live or dead. No such line: seedprobe no-seed-line <file>:<line>, exit 2.
  Exit 0: verdicts. Exit 2: bad-spec, no-rebuild, no-seed-line (no build, or no write). Exit 3: tool-fault (text on stdout).
  Every edit is restored with git checkout on every exit after the first edit.
"""
import hashlib
import importlib.util
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
OUT = REPO / "build3ds" / "out"
SEED_RX = re.compile(r"^(.+):(\d+)@(-|\d+)$")
DRAW_RX = re.compile(r"^draw \d+")


class Stop(Exception):
    def __init__(self, code, text):
        Exception.__init__(self, text)
        self.code = code
        self.text = text


def load_bench():
    spec = importlib.util.spec_from_file_location("bench", str(HERE / "bench.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def run_to(cmd, outfile, env=None):
    """Run cmd in the worktree root, stdout and stderr to outfile. Returns the exit code."""
    with open(str(outfile), "w", encoding="utf-8") as f:
        return subprocess.run(cmd, cwd=str(REPO), stdout=f, stderr=subprocess.STDOUT, env=env).returncode


def sha1_of(p):
    return hashlib.sha1(p.read_bytes()).hexdigest() if p.is_file() else None


def parse_seed(s):
    """(file, line, frame, old, new) for a seed text, else None."""
    parts = s.split("|")
    if len(parts) != 3:
        return None
    m = SEED_RX.match(parts[0])
    if not m or not parts[1]:
        return None
    return m.group(1), int(m.group(2)), m.group(3), parts[1], parts[2]


def frame_section(lines, f):
    """The draw lines of frame f: from `frame f` to the next `frame` line. None when drec.txt has no `frame f` line."""
    want = ["frame", str(f)]
    sec, on, found = [], False, False
    for ln in lines:
        tok = ln.split()
        if tok[:1] == ["frame"]:
            if on:
                break
            on = tok[:2] == want
            found = found or on
            continue
        if on:
            sec.append(ln)
    return sec if found else None


def write_verdicts(seeds_file, verdicts):
    """--write: set the last field of the seeds-file line for each (file, line, word)."""
    p = Path(seeds_file)
    lines = p.read_text(encoding="utf-8").splitlines(keepends=True)
    for f, line, word in verdicts:
        stem = Path(f).stem
        idx = None
        for i, raw in enumerate(lines):
            fields = raw.split("|")
            if len(fields) >= 4 and fields[0].strip() == stem and fields[3].strip() == str(line):
                idx = i
                break
        if idx is None:
            raise Stop(2, "seedprobe no-seed-line %s:%d" % (f, line))
        raw = lines[idx]
        eol = raw[len(raw.rstrip("\r\n")):]
        body = raw[:len(raw) - len(eol)]
        cut = body.rfind("|")
        tail = body[cut + 1:]
        lead = tail[:len(tail) - len(tail.lstrip())]
        trail = tail[len(tail.rstrip()):]
        lines[idx] = body[:cut + 1] + lead + word + trail + eol
    p.write_text("".join(lines), encoding="utf-8", newline="")


def main(argv):
    if len(argv) < 2:
        print("usage: python 3ds/tools/seedprobe.py <bench> <seed> [<seed> ...] [--write <seeds file>]", file=sys.stderr)
        return 2
    bench, seeds, write, i = argv[0], [], None, 1
    while i < len(argv):
        if argv[i] == "--write" and i + 1 < len(argv):
            write = argv[i + 1]
            i += 2
            continue
        seeds.append(argv[i])
        i += 1
    row, lane = os.environ.get("PM_ROW"), os.environ.get("PM_LANE")
    if not row or not lane:
        print("seedprobe needs PM_ROW and PM_LANE set. Nothing was run.")
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    py = sys.executable
    bench_py = str(HERE / "bench.py")
    edited = []
    try:
        # b. validate every seed before any edit (no build on a bad spec)
        plan = []
        for s in seeds:
            ps = parse_seed(s)
            if ps is None:
                m = s.split("|")[0]
                raise Stop(2, "seed %s bad-spec" % m)
            f, line, frame, old, new = ps
            path = REPO / f
            if not path.is_file():
                raise Stop(2, "seed %s:%d bad-spec" % (f, line))
            plan.append((f, line, frame, old, new))
        # reads each file once, applies the seeds in order on the in-memory lines
        texts = {}
        for f, line, frame, old, new in plan:
            if f not in texts:
                texts[f] = (REPO / f).read_text(encoding="utf-8", newline="").splitlines(keepends=True)
            ls = texts[f]
            if not (1 <= line <= len(ls)) or ls[line - 1].count(old) != 1:
                raise Stop(2, "seed %s:%d bad-spec" % (f, line))
            ls[line - 1] = ls[line - 1].replace(old, new, 1)
        if write:
            # no-seed-line is checked before the build too
            wl = Path(write).read_text(encoding="utf-8").splitlines()
            for f, line, frame, old, new in plan:
                stem = Path(f).stem
                if not any(len(x.split("|")) >= 4 and x.split("|")[0].strip() == stem and x.split("|")[3].strip() == str(line) for x in wl):
                    raise Stop(2, "seedprobe no-seed-line %s:%d" % (f, line))
        # a. base (stored: no run)
        code = run_to([py, bench_py, "base", bench, "--row", row], OUT / "seedprobe_base.txt")
        if code != 0:
            raise Stop(3, "seedprobe tool-fault base rc %d, see build3ds/out/seedprobe_base.txt" % code)
        # the files must be clean before the probe (restore uses git checkout)
        files = sorted(set(f for f, *_ in plan))
        st = subprocess.run(["git", "status", "--porcelain", "--"] + files, cwd=str(REPO), capture_output=True, text=True).stdout
        if st.strip():
            raise Stop(3, "seedprobe tool-fault seed file not clean before the probe: %s" % st.strip())
        # c. edit, sha1, build, sha1
        for f in files:
            (REPO / f).write_text("".join(texts[f]), encoding="utf-8", newline="")
            edited.append(f)
        elf = REPO / "build3ds" / ("lane-" + lane) / "pm_3ds.elf"
        before = sha1_of(elf)
        benv = dict(os.environ, PM_ROW=row, PM_LANE=lane)
        code = run_to(["sh", "3ds/build.sh"], OUT / "seedprobe_build.txt", benv)
        if code != 0:
            raise Stop(3, "seedprobe tool-fault build rc %d, see build3ds/out/seedprobe_build.txt" % code)
        if sha1_of(elf) == before:
            raise Stop(2, "seedprobe no-rebuild")
        # d. run, cmp
        code = run_to([py, bench_py, "run", bench, "--row", row, "--why", "seedprobe"], OUT / "seedprobe_run.txt")
        if code != 0:
            raise Stop(3, "seedprobe tool-fault run rc %d, see build3ds/out/seedprobe_run.txt" % code)
        B = load_bench()
        rr = B.row_shots_run(bench, row)
        if rr is None:
            raise Stop(3, "seedprobe tool-fault no run record of %s under row %s with shots" % (bench, row))
        code = run_to([py, bench_py, "cmp", bench, "--row", row], OUT / "seedprobe_cmp.txt")
        cmp_text = (OUT / "seedprobe_cmp.txt").read_text(encoding="utf-8", errors="replace")
        if "cold start" in cmp_text or code not in (0, 1):
            raise Stop(3, "seedprobe tool-fault cmp rc %d, see build3ds/out/seedprobe_cmp.txt" % code)
        cmp_equal = code == 0
    finally:
        # e. restore, always
        if edited:
            r = subprocess.run(["git", "checkout", "--"] + edited, cwd=str(REPO), capture_output=True, text=True)
            if r.returncode != 0:
                raise Stop(3, "seedprobe tool-fault restore failed: %s" % r.stderr.strip())
    # f. verdicts
    base_commit = B.base_commit(None)
    base_drec = B.store_root() / (base_commit or "?") / bench / "drec.txt"
    row_drec = Path(rr["shot_path"]) / "drec.txt"
    if not base_commit or not base_drec.is_file() or not row_drec.is_file():
        raise Stop(3, "seedprobe tool-fault no drec.txt in base %s or row %s" % (base_drec, row_drec))
    bl = base_drec.read_text(encoding="utf-8", errors="replace").split("\n")
    rl = row_drec.read_text(encoding="utf-8", errors="replace").split("\n")
    out, verdicts = [], []
    for f, line, frame, old, new in plan:
        if frame == "-":
            word = "dead" if cmp_equal else "live"
            detail = "window equal" if cmp_equal else "window differs"
        else:
            bs, rs = frame_section(bl, frame), frame_section(rl, frame)
            if bs is None or rs is None:
                raise Stop(3, "seedprobe tool-fault no frame %s in drec.txt (base %s, row %s)" % (frame, bs is not None, rs is not None))
            # Correction 1 (ORACLE8 2026-11-13): each `draw <n>` line reads as `draw` (drec numbers draws over the whole window);
            # the draw count of the frame still counts (the number of draw lines must match).
            norm = lambda sec: [DRAW_RX.sub("draw", x) for x in sec]
            same = norm(bs) == norm(rs) and sum(x.startswith("draw ") for x in bs) == sum(x.startswith("draw ") for x in rs)
            word = "dead" if same else "live"
            detail = ("frame %s equal" if word == "dead" else "frame %s differs") % frame
        out.append("seed %s:%d %s (%s)" % (f, line, word, detail))
        verdicts.append((f, line, word))
    if write:
        write_verdicts(write, verdicts)
    for ln in out:
        print(ln)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except Stop as e:
        print(e.text)
        sys.exit(e.code)

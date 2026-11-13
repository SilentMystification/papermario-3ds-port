#!/usr/bin/env python3
"""gold.py: coverage harness of a GOLD row (plan GMHARN). Gives the 90 % verdict.

usage (repo root; Windows: py -3 3ds/tools/gold.py ...):
  gold.py <row> [--no-build]

Reads 3ds/tools/cov.d/<row>.txt (one source file per line; # comments and blank lines skipped)
and the coverage benches of 3ds/tools/bench.d/<row>.json (the benches tagged fncov).
1. Without --no-build: PM_FNCOV=1 PM_LANE=<row> sh 3ds/build.sh (census lane <row>_fncov). Exit not 0: print, exit 1.
2. For each coverage bench: python 3ds/tools/bench.py run <bench> --row <row> --lane <row>_fncov --why "gold coverage".
   It takes the last runs.jsonl line with that bench and row, and copies its log_path to build3ds/out/<row>_<bench>.log.
3. covsites.py on all copies (--lane <row>): the sites h and t are summed over the files.
   Verdict: 10 * H >= NEED * T is pass (NEED = 9, i.e. 90 %), else fail with the cov miss lines.
   Exit 0 on pass, 1 on fail. It does not run bench.py base.
"""
import json, os, re, shutil, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
NEED = 9  # pass when 10 * H >= NEED * T (90 %)


def fail(msg):
    print(msg)
    sys.exit(1)


def read_files(row):
    p = HERE / "cov.d" / (row + ".txt")
    if not p.is_file():
        fail("gold: no %s" % p)
    out = []
    for ln in p.read_text(encoding="utf-8").splitlines():
        ln = ln.strip()
        if ln and not ln.startswith("#"):
            out.append(ln)
    if not out:
        fail("gold: %s lists no source file" % p)
    return out


def cov_benches(row):
    p = HERE / "bench.d" / (row + ".json")
    if not p.is_file():
        fail("gold: no %s" % p)
    benches = json.loads(p.read_text(encoding="utf-8")).get("benches", [])
    out = [b["name"] for b in benches if "fncov" in (b.get("tags") or [])]
    if not out:
        fail("gold: %s has no fncov bench" % p)
    return out


def env_for(row):
    env = dict(os.environ)
    env["PM_ROW"] = row
    env["PM_LANE"] = row
    return env


def build(row):
    env = env_for(row)
    env["PM_FNCOV"] = "1"
    r = subprocess.run(["sh", "3ds/build.sh"], cwd=str(REPO), env=env)
    if r.returncode != 0:
        fail("gold %s: build exit %d" % (row, r.returncode))


def last_log_path(row, bench):
    found = None
    with open(REPO / "3ds/logs/perf/runs.jsonl", encoding="utf-8") as fh:
        for ln in fh:
            ln = ln.strip()
            if not ln:
                continue
            r = json.loads(ln)
            if r.get("bench") == bench and r.get("row") == row:
                found = r.get("log_path")
    if not found:
        fail("gold %s: no runs.jsonl line for bench %s" % (row, bench))
    return found


def run_bench(row, bench):
    r = subprocess.run([sys.executable, "3ds/tools/bench.py", "run", bench, "--row", row,
                        "--lane", row + "_fncov", "--why", "gold coverage"],
                       cwd=str(REPO), env=env_for(row))
    if r.returncode != 0:
        fail("gold %s: bench run %s exit %d" % (row, bench, r.returncode))
    src = REPO / last_log_path(row, bench)
    if not src.is_file():
        fail("gold %s: log copy source missing: %s" % (row, src))
    out_dir = REPO / "build3ds" / "out"
    out_dir.mkdir(parents=True, exist_ok=True)
    dst = out_dir / ("%s_%s.log" % (row, bench))
    shutil.copyfile(str(src), str(dst))
    return str(dst)


def main(argv):
    if not argv or argv[0].startswith("-"):
        fail("usage: gold.py <row> [--no-build]")
    row = argv[0]
    no_build = "--no-build" in argv[1:]
    files = read_files(row)
    benches = cov_benches(row)
    if not no_build:
        build(row)
    logs = [run_bench(row, b) for b in benches]
    r = subprocess.run([sys.executable, "3ds/tools/covsites.py", *logs, "--lane", row, *files],
                       cwd=str(REPO), capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    if r.returncode != 0:
        fail("gold %s: covsites exit %d" % (row, r.returncode))
    h = t = 0
    misses = []
    for ln in r.stdout.splitlines():
        m = re.match(r"^cov (\S+) sites (\d+)/(\d+) funcs", ln)
        if m:
            h += int(m.group(2))
            t += int(m.group(3))
        elif ln.startswith("cov miss "):
            misses.append(ln)
    if 10 * h >= NEED * t:
        print("gold %s sites %d/%d pass" % (row, h, t))
        return 0
    print("gold %s sites %d/%d fail" % (row, h, t))
    for ln in misses:
        print(ln)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

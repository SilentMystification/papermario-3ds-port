"""capture_check.py - before a console log push: run the same switch words in Azahar (bench capchk, mac_01 fxtest=87) with the
worst saved settings (options.ini log_on=0, profmode=prof, live and disk level off) and check the proof lines in the game log.
Prints one line: CAPTURE CHECK OK, CAPTURE CHECK FAIL or CAPTURE CHECK TOOL FAULT.

Usage (repo root, Git Bash): python 3ds/tools/capture_check.py --row <id> --proof "grabw,fbbind" [--profile log] [game words ...]
       python 3ds/tools/capture_check.py --row <id> --proof "<lines>" --raw "<exact switch words>"   (no push_3ds.sh --dry)
The words come from `sh 3ds/push_3ds.sh --dry --profile <p> <game words>` (the 3dslink argv after "--verbose").
Exit 0 OK, 1 FAIL, 2 tool fault.
"""
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RUNS = REPO / "3ds" / "logs" / "perf" / "runs.jsonl"
GATE = "options: levels run log_on=1"  # pm_options.c:249, dropped when the log gate is off


def words_from_dry(out):
    """The game words of a push_3ds.sh --dry output: the text after ' -- --verbose ' on the '3dslink argv:' line."""
    for line in out.splitlines():
        if line.startswith("3dslink argv:") and " -- --verbose " in line:
            return line.split(" -- --verbose ", 1)[1].strip()
    return None


PROF_LINE = re.compile(r"prof f\d+ w\d+")


def judge(text, proof, prof=False):
    """(ok, detail) for a game log text and a list of proof substrings. prof: the launch words set prof=<n>, which turns logging off
    (pm_options.c:284), so the gate is a prof line and the fxtest: done line instead of log_on=1."""
    if prof:
        miss = [p for p in proof if p not in text]
        if PROF_LINE.search(text) and "fxtest: done" in text and not miss:
            return True, "prof run, prof line and fxtest: done, proof " + " ".join("%s %d" % (p, text.count(p)) for p in proof)
        why = ["prof run, no prof line" if not PROF_LINE.search(text) else "prof run, no fxtest: done line"]
        if miss:
            why.append("missing proof: " + ", ".join(miss))
        return False, ", ".join(why)
    gate = GATE in text
    miss = [p for p in proof if p not in text]
    if gate and not miss:
        return True, "log gate on, proof " + " ".join("%s %d" % (p, text.count(p)) for p in proof)
    why = []
    if not gate:
        why.append('log gate OFF (no "%s" line)' % GATE)
    if miss:
        why.append("missing proof: " + ", ".join(miss))
    return False, ", ".join(why)


def last_run(bench, row):
    best = None
    if RUNS.exists():
        for line in RUNS.read_text(encoding="utf-8").splitlines():
            try:
                r = json.loads(line)
            except ValueError:
                continue
            if r.get("bench") == bench and r.get("row") == row:
                best = r
    return best


def main(argv=None):
    ap = argparse.ArgumentParser(description="Azahar check of a console capture launch line.")
    ap.add_argument("--row", required=True)
    ap.add_argument("--proof", required=True, help="comma list of text that must be in the game log")
    ap.add_argument("--profile", default="log")
    ap.add_argument("--raw", default="", help="exact switch words (skips push_3ds.sh --dry)")
    ap.add_argument("words", nargs="*")
    a = ap.parse_args(argv)
    proof = [p.strip() for p in a.proof.split(",") if p.strip()]
    words = a.raw.strip()
    if not words:
        p = subprocess.run(["sh", "3ds/push_3ds.sh", "--dry", "--profile", a.profile] + a.words, cwd=REPO, capture_output=True, text=True)
        words = words_from_dry(p.stdout)
        if not words:
            print("CAPTURE CHECK TOOL FAULT: no '3dslink argv:' line from push_3ds.sh --dry (rc %d)" % p.returncode)
            return 2
    scen = REPO / "3ds" / "tools" / "azt.d" / "CAPCHK.json"
    keep = scen.read_bytes() if scen.exists() else None
    prof = any(w.startswith("prof=") and w not in ("prof=off", "prof=0") for w in words.split())
    if prof and keep is not None:  # a prof run has no log_on line: its boot check is the prof line (judge prof=True)
        bs = bytes([92])  # one backslash byte: the JSON file escapes each one
        scen.write_bytes(keep.replace(b'"regex": "build ' + bs * 2 + b'd+' + bs * 2 + b'S* git"',
                                      b'"regex": "prof f' + bs * 2 + b'd+ w' + bs * 2 + b'd+"', 1))
    before = last_run("capchk", a.row)
    env = dict(os.environ, PM_DEBUG_ADD=words, PM_OPT_WORST="1", PM_ROW=a.row)
    rc = subprocess.run([sys.executable, "3ds/tools/bench.py", "run", "capchk", "--row", a.row, "--why", "capture check: " + words],
                        cwd=REPO, env=env).returncode
    if keep is not None and scen.read_bytes() != keep:
        scen.write_bytes(keep)  # azt.ps1 marks a scenario without its until line unverified: the check leaves no file change
    rec = last_run("capchk", a.row)
    if not rec or (before and rec.get("run_id") == before.get("run_id")):
        print("CAPTURE CHECK TOOL FAULT: bench.py run capchk rc %d, no new run record" % rc)
        return 2
    log = Path(rec["log_path"])
    if not log.is_absolute():
        log = REPO / log
    ok, detail = judge(log.read_text(encoding="utf-8", errors="replace") if log.exists() else "", proof, prof=prof)
    if ok and rc != 0:
        print("CAPTURE CHECK TOOL FAULT: bench.py run capchk rc %d with every proof line (run %s, %s)" % (rc, rec.get("run_id"), rec["log_path"]))
        return 2
    # With the log gate off the until line can miss (rc 10): the judge still names the cause.
    print('CAPTURE CHECK %s: words "%s", %s, bench rc %d (run %s, %s)' % ("OK" if ok else "FAIL", words, detail, rc, rec.get("run_id"), rec["log_path"]))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

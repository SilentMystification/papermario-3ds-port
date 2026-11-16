# ws_CAPTOOL code (exact text; the goal page is ws_CAPTOOL.md)
## A. 3ds/push_3ds.sh
Line 386 before: `i=1`. After: the new line `set -- --verbose livelog "$@" # CAPTOOL: one argv: the echo prints what 3dslink gets`, then `i=1`.
Line 394 before: `        echo "+ $LINK -a $ip $SRC3DSX -- --verbose $*"`. After: `        echo "+ $LINK -a $ip $SRC3DSX -- $*"`
Line 395 before: `... "$SRC3DSX" -- --verbose livelog "$@" >"$outfile" 2>&1 &`. After: `... "$SRC3DSX" -- "$@" >"$outfile" 2>&1 &`
Line 400 before: `        echo "+ docker run devkitpro/devkitarm 3dslink -a $ip /out/$src_name -- --verbose $*"`. After: the same with `-- $*"`.
Line 402 before: `... "/out/$src_name" -- --verbose livelog "$@" >"$outfile" 2>&1 &`. After: `... "/out/$src_name" -- "$@" >"$outfile" 2>&1 &`
## B. 3ds/tools/pm_logrecv.py, after line 28 `KIND = {...}`
```python
HELLO = b"#PMLIVE "


def connect_diag(ip, port, cls):
    """CAPTOOL: one line for a failed connect. It never says that the console is down."""
    if cls == "refused":
        return "[logrecv] refused on %s:%d: nothing listens: the game is not running, or it started without the livelog word" % (ip, port)
    return ("[logrecv] no answer on %s:%d (%s): the game has no live socket (livelog missing), or the accept loop ended, "
            "or the network path: see log.old/FTP" % (ip, port, cls))


def hello_diag(line):
    """CAPTOOL: the first game line of a connection (pm_livesrv.h). A warning when the game logs nothing, else None."""
    if not line.startswith(HELLO):
        return None
    f = line.split()
    kv = dict(zip(f[1::2], f[2::2]))
    if kv.get(b"log_on") == b"0" and kv.get(b"prof") == b"0":
        return ("[live] GAME LOGGING OFF (%s): no log line follows. Add the launch word log=on (push --profile log has it) "
                "or set Options > Logging On" % line.decode("ascii", "replace"))
    return None
```
## C. pm_logrecv.py receive(): before line 84 `                    live.write(line + b"\n")` add (20 spaces):
`d = hello_diag(line)` / `if d:` / `    print(d, flush=True)`
## D. pm_logrecv.py main(). After line 124 (`--out`) add:
`    ap.add_argument("--port", type=int, default=PORT)  # CAPTOOL: the tests use a local port`
`    ap.add_argument("--retry", type=float, default=0.0, help="retry the connect for this many seconds, 1 s apart")`
Replace lines 127-131 (`try:` ... `return 1`) with:
```python
    end = time.monotonic() + args.retry
    while True:
        try:
            sock = socket.create_connection((args.ip, args.port), timeout=8)
            break
        except OSError as e:
            cls = "refused" if isinstance(e, ConnectionRefusedError) else "timeout" if isinstance(e, socket.timeout) else "error"
            if time.monotonic() >= end:
                print(connect_diag(args.ip, args.port, cls), flush=True)
                return 1
            time.sleep(1)
```
Line 3 (usage) after: `Usage: python 3ds/tools/pm_logrecv.py --ip <3ds-ip> [--label o3ds] [--timeout 60] [--out build3ds] [--retry 0] [--port 17492]`
## E. 3ds/tools/livelog.py, after line 31 `import time` add:
`sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))` and `import pm_logrecv  # noqa: E402  CAPTOOL: hello_diag`
Lines 101-102 (probe) after:
```python
    print("[livelog] probe %s:%d connected: the TCP connect completed. A game that sends no #PMLIVE line at once never accepts "
          "(a build before CAPLOG with logging off: options.ini log_on=0 and no log=on word)" % (ip, PORT), flush=True)
```
## F. livelog.py read loop: after line 201 `            for line in lines:` add (16 spaces):
```python
                if line.startswith(b"#PMLIVE "):  # CAPTOOL: the game status line, not a log line (report_log counts file lines)
                    print("[livelog] %s" % line.decode("ascii", "replace"), flush=True)
                    d = pm_logrecv.hello_diag(line)
                    if d:
                        print(d, flush=True)
                    continue
```
## G. 3ds/tools/test_pm_logrecv.py: add `import contextlib` and `import io` to the imports, these 2 functions before `def main():`,
and the calls `check_hello()` and `check_retry(base)` after `check_closed(base)`.
```python
def check_hello():
    off = pm_logrecv.hello_diag(b"#PMLIVE log_on 0 live 3 disk 3 prof 0")
    assert off and off.startswith("[live] GAME LOGGING OFF (#PMLIVE log_on 0 live 3 disk 3 prof 0)"), off
    assert pm_logrecv.hello_diag(b"#PMLIVE log_on 1 live 3 disk 3 prof 0") is None
    assert pm_logrecv.hello_diag(b"#PMLIVE log_on 0 live -1 disk -1 prof 1") is None
    assert pm_logrecv.hello_diag(b"#12 build 5824") is None
    want = ("[logrecv] no answer on 192.168.1.131:17492 (timeout): the game has no live socket (livelog missing), "
            "or the accept loop ended, or the network path: see log.old/FTP")
    assert pm_logrecv.connect_diag("192.168.1.131", 17492, "timeout") == want


def check_retry(out_dir):
    # Seed: no retry to a closed port gives the refused line and exit 1. Then --retry reaches a server that starts 1.5 s late.
    p = socket.socket()
    p.bind(("127.0.0.1", 0))
    port = p.getsockname()[1]
    p.close()
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = pm_logrecv.main(["--ip", "127.0.0.1", "--port", str(port), "--out", out_dir, "--label", "R0"])
    assert rc == 1, rc
    assert buf.getvalue().startswith("[logrecv] refused on 127.0.0.1:%d: nothing listens" % port), buf.getvalue()

    def late():
        time.sleep(1.5)
        ls = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        ls.bind(("127.0.0.1", port))
        ls.listen(1)
        try:
            serve(ls, build_stream(True, b"#PMLIVE log_on 1 live 3 disk 3 prof 0\n"))
        finally:
            ls.close()
    t = threading.Thread(target=late, daemon=True)
    t.start()
    rc = pm_logrecv.main(["--ip", "127.0.0.1", "--port", str(port), "--out", out_dir, "--label", "R1", "--retry", "8", "--timeout", "20"])
    t.join(timeout=20)
    assert rc == 0, rc
    assert read_file(os.path.join(out_dir, "hw_R1_log.txt")) == LOG_PAYLOAD
```
## H. 3ds/tools/capture_check.py (new)
```python
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


def judge(text, proof):
    """(ok, detail) for a game log text and a list of proof substrings."""
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
    ok, detail = judge(log.read_text(encoding="utf-8", errors="replace") if log.exists() else "", proof)
    if ok and rc != 0:
        print("CAPTURE CHECK TOOL FAULT: bench.py run capchk rc %d with every proof line (run %s, %s)" % (rc, rec.get("run_id"), rec["log_path"]))
        return 2
    # With the log gate off the until line can miss (rc 10): the judge still names the cause.
    print('CAPTURE CHECK %s: words "%s", %s, bench rc %d (run %s, %s)' % ("OK" if ok else "FAIL", words, detail, rc, rec.get("run_id"), rec["log_path"]))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
```
## I. 3ds/tools/test_capture_check.py (new)
```python
"""test_capture_check.py - host test of capture_check.words_from_dry and judge (no Azahar). Prints a line, exit 1 on fail."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capture_check as cc  # noqa: E402

DRY = ("dry run: nothing built, nothing sent\n3dslink argv: /opt/devkitpro/tools/bin/3dslink -a 192.168.1.131 "
       "build3ds/pm_3ds.3dsx -- --verbose livelog grablog fbbind log=on prof=off\n")


def main():
    w = cc.words_from_dry(DRY)
    assert w == "livelog grablog fbbind log=on prof=off", w
    assert cc.words_from_dry("no argv line\n") is None
    ok, d = cc.judge("#5 options: levels run log_on=1 live=3\n#9 grabw f1\n#10 fbbind n0\n#11 grabw f2\n", ["grabw", "fbbind"])
    assert ok and d == "log gate on, proof grabw 2 fbbind 1", d
    ok, d = cc.judge("#1 build 5824-x git 6ef6af53c7 loglevel 3\n#2 fxtest: done\n", ["grabw"])  # seed: log_on 0 drops the gate line
    assert not ok and d == 'log gate OFF (no "options: levels run log_on=1" line), missing proof: grabw', d
    print("captool capture_check ok")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as e:
        print("FAIL: %s" % (e,))
        sys.exit(1)
```
## J. azt.ps1 and run_azahar.ps1
azt.ps1:520 before: `$dbgArg = [string]$sc.debug`. After:
`$dbgArg = [string]$sc.debug; if ($env:PM_DEBUG_ADD) { $dbgArg = "$($env:PM_DEBUG_ADD) $dbgArg" } # CAPTOOL: capture_check.py puts the push words first (the console reads its launch words before debug3ds.txt)`
run_azahar.ps1: after line 290 (the `}` that ends the `profmode=off` block) add:
```powershell
if ($env:PM_OPT_WORST) { # CAPTOOL: the worst saved settings (logging off, Profiling prof, live and disk level off); the launch words must win
    if (-not (Test-Path $optFile)) { Set-Content $optFile "version=2`r`n" -NoNewline; Copy-Item $optFile $optBak -Force }
    $optText = Get-Content $optFile -Raw
    foreach ($kv in @("log_on=0", "profmode=prof", "live_level=-1", "disk_level=-1")) {
        $k = $kv.Split("=")[0]
        if ($optText -match "(?m)^$k=[^\r\n]*") { $optText = $optText -replace "(?m)^$k=[^\r\n]*", $kv }
        else { $optText = $optText.TrimEnd() + "`r`n$kv`r`n" }
    }
    Set-Content $optFile $optText -NoNewline
    Write-Output "options.ini: log_on=0 profmode=prof live_level=-1 disk_level=-1 for this run (PM_OPT_WORST)"
}
```
## K. New files
3ds/tools/bench.d/CAPCHK.json:
```json
{"benches": [{"name": "capchk", "kind": "azahar_stream", "speed": "turbo", "default": false,
  "purpose": "CAPTOOL capture check: capture_check.py runs it with PM_DEBUG_ADD (the push words) and PM_OPT_WORST (options.ini log_on=0, profmode=prof). Log only.",
  "scenario": "capchk_mac01", "cap": 25, "until": "fxtest: done", "tags": ["azahar"], "gates": []}]}
```
3ds/tools/azt.d/CAPCHK.json:
```json
{"scenarios": [{"name": "capchk_mac01", "status": "unverified",
  "reason": "CAPTOOL: capture_check.py adds the push words (PM_DEBUG_ADD) before these words.",
  "subject": "mac_01 (entry 0, pos 0 0 200), fxtest=87 (fx_underwater) spawned at frame 402.",
  "proves": "CAPTOOL: with options.ini log_on=0 and profmode=prof the launch words still give the debug lines (options: levels run log_on=1, fxtest: spawn 87).",
  "debug": "warp=mac_01:0 pos=0:0:200 story=ch0_toad_town fxtest=87 fixedseed det", "extra": ["-Turbo"],
  "until": "fxtest: done", "cap": 25, "deadline": 20, "first_marker": "build \\d+ git", "expects": "warp", "needs_picture": false,
  "expected_events": [{"name": "boot", "regex": "build \\d+ git"}, {"name": "done", "regex": "fxtest: done"}]}]}
```
The first marker and the until line pass with logging off: `build N git` is logged before pm_options_init (pm_main.c:942, :946), `fxtest: done` is an event line (pm_fxtest.c:287).
3ds/tests/host.d/captool.sh (LF endings):
```sh
#!/bin/sh
# CAPTOOL: pm_logrecv diagnosis, retry and #PMLIVE (test_pm_logrecv.py), capture_check judge (test_capture_check.py),
# and the push echo prints the real 3dslink argv. Seed: the old echo line fails the argv check.
python3 3ds/tools/test_pm_logrecv.py || exit 1
python3 3ds/tools/test_capture_check.py || exit 1
chk() { grep -qF 'set -- --verbose livelog "$@"' "$1" && ! grep -qF -- '-- --verbose $*"' "$1"; }
printf '%s\n' 'set -- --verbose livelog "$@"' 'echo "+ $LINK -a $ip $SRC3DSX -- --verbose $*"' > /tmp/captool_seed.sh
if chk /tmp/captool_seed.sh; then echo "captool seed FAIL: the old echo passes"; exit 1; fi
chk 3ds/push_3ds.sh || { echo "captool FAIL: push echo argv"; exit 1; }
grep -qF 'if line.startswith(b"#PMLIVE "):' 3ds/tools/livelog.py || { echo "captool FAIL: livelog #PMLIVE"; exit 1; }
echo "captool ok"
```

"""Run the repo's splat asset extraction and generate build.ninja (no N64 compile).
Run from the repo root with the venv python: see extract_assets.ps1."""
import subprocess
import sys
from pathlib import Path

# configure probes `iconv` at import time; treat a missing iconv as "not working" (it falls back to python)
_run = subprocess.run
def _safe_run(cmd, *a, **k):
    try:
        return _run(cmd, *a, **k)
    except FileNotFoundError:
        return subprocess.CompletedProcess(cmd, 127, b"", b"")
subprocess.run = _safe_run

root = Path.cwd()
sys.path[:0] = [str(root / "tools/configure/src/configure"), str(root / "tools/build")]
sys.path.append(str(root / "tools/splat_ext"))
import main

# skip the Rust tool version check and the tools/splat make step (splat is the pip package)
main.exec_shell = lambda cmd: "tool 99.0.0" if "--version" in cmd else ""
main.PIGMENT64 = f'{sys.executable} {root / "3ds/tools/pigment64_shim.py"}'  # no quotes: cmd /c chokes
sys.argv = ["configure", "us"]
main.main()

# ninja runs under cmd on Windows: point $python at the venv python
p = root / "build.ninja"
shim = root / "3ds/tools/crunch64_shim.py"
p.write_text(
    p.read_text(encoding="utf-8")
    .replace("python = python3", f"python = {sys.executable}")
    .replace("crunch64 compress", f"{sys.executable} {shim} compress"),
    encoding="utf-8",
)

# phony target: generated headers + converted images (no code compile)
import re
import subprocess

out = subprocess.run(["ninja", "-t", "targets", "all"], capture_output=True, text=True, check=True).stdout
t = [l.split(":")[0] for l in out.splitlines()]
t = [x for x in t if re.search(r"(\.(h|png\.bin|pal\.bin)|build/include/.*\.inc\.c)$", x.replace("\\", "/")) and not x.startswith(("src/", "ver/us/build/src"))]
with open(p, "a", encoding="utf-8") as f:
    f.write("\nbuild e01_assets: phony " + " ".join(t) + "\n")
print(f"e01_assets: {len(t)} targets")


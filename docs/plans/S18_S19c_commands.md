# S18 and S19c test commands (prepared, not run)

Repo root: `c:\Users\Andrew\Documents\GitHub\papermario-3ds-port`. Run every command from the repo root in PowerShell. Do not run two Azahar runs at once (`run_azahar.ps1` takes a lock, exit 3 = lock timeout).

Exit code rules (from `3ds/run_azahar.ps1`):
- Exit 0 does NOT prove a match for `-Until` runs (no `-Smoke`). The script exits 0 for any stop reason except unmapped access or crash. Read the stdout line `matched '<regex>'` for a match.
- With `-Smoke`, exit 0 = `smoke done` seen and no unmapped access. Exit 1 = smoke not reached. Exit 2 = unmapped access or crash.
- `build3ds/last_log.txt` = game log. `build3ds/last_unmapped.txt` = unmapped accesses (must be empty). Shots go to `build3ds/shots/NNNN.ppm` (copied from the SD card).
- `game mode <n> frame <f>` lines come from `pm_map.c:614`. Mode numbers: 2 title, 4 world, 8 battle, 9 end battle, 10 pause, 11 unpause, 12 language select, 14 file select, 16 intro, 17 demo (`src/game_modes.h`).

## S18a: pause is real

Command (fixed 300 frames, pause opens by padscript):
```
powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Stall 40 -Debug "fast preset=toad padscript=wait80,START*3,wait100"
```
Pixel check (four pause frames, each one Azahar run, about 25 s each):
```
python 3ds/tools/golden.py pause1 pause2 pause3 pause4
```
Pass:
- stdout has `matched 'smoke done'`, exit 0.
- `build3ds/last_log.txt` has `game mode 10 frame`. `build3ds/last_unmapped.txt` is empty.
- golden table: each row `ok`, exit 0. (Golden references `3ds/golden/pause1..4.png` were saved in G01g. This proves the pause draws the same frame again. It does not prove the first draw is correct, so keep a by-eye check out of scope.)
Fail: no `game mode 10` line, any `DIFF` row (exit 1), or `RUN FAILED` (exit 2).

## S18b: demo state starts from the title timer

Command (no padscript, title timer starts the demo under `fast`):
```
powershell -File 3ds/run_azahar.ps1 -Turbo -FaultStop -Stall 40 -Timeout 400 -Until "game mode 17 frame" -Debug "fast"
```
Pass:
- stdout has `matched 'game mode 17 frame'`.
- `build3ds/last_log.txt` has `game mode 2 frame` before it and `game mode 17 frame` once. `build3ds/last_unmapped.txt` is empty.
- No line `[CRASH]` and no `assert` in the log (`Select-String` or `grep -c` = 0).
Fail: no match before timeout (the title timer did not start the demo), exit 2 (fault), or unmapped lines.
Limit: this run checks the start of the demo only. The later demo scenes and the loop repeat (MS01) are not in this check.

## S18c: language select

Not runnable in the US build. The only call site is `src/state_title_screen.c:385`, under `#if VERSION_PAL`. The PAL build is not built (`docs/HISTORY.md` S18 note). Check for the US build: run the S18b command and count `game mode 12 frame` lines in `build3ds/last_log.txt`. Expected count = 0. The row can close only as `n/a in US build` after the user agrees. Needs a user decision (QUESTIONS.md).

## S19c: black and olive polygons in attacks

Command (one attack-heavy battle: Goomba with Jump, `autoa` presses A through the battle, 500 frames, 300 shots from absolute frame 160):
```
powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Stall 40 -Timeout 400 -Debug "fast autoa preset=toad battle battlef=100 smokeframes=500 shots shotwin=160:300"
```
Text evidence (grep `build3ds/last_log.txt`):
- `debug battle: formation` appears once (battle started).
- `gbi skip <op>` lines show which Fast3D opcodes the battle used but the port does not handle. A listed op is a lead, not proof of the polygon cause.
- `smoke done` present and `build3ds/last_unmapped.txt` empty.

Pixel check (no picture reading). Save this as a new file `3ds/tools/poly_scan.py` (the scan does not exist yet) and run it:
```
python 3ds/tools/poly_scan.py
```
Script content:
```python
import glob, sys
import numpy as np
from PIL import Image

def olive(a):  # approximate olive, exact RGB still to confirm on one by-eye shot
    r, g, b = a[..., 0].astype(int), a[..., 1].astype(int), a[..., 2].astype(int)
    return (r >= 100) & (r <= 160) & (g >= 100) & (g <= 160) & (b <= 60)

def black8(a):  # count 8x8 solid black blocks
    m = (a.max(axis=2) == 0).astype(np.int32)
    c = np.pad(m.cumsum(0).cumsum(1), ((1, 0), (1, 0)))
    s = c[8:, 8:] - c[:-8, 8:] - c[8:, :-8] + c[:-8, :-8]
    return int((s == 64).sum())

gold = np.asarray(Image.open("3ds/golden/battle.png").convert("RGB"))
base_b, base_o = black8(gold), int(olive(gold).sum())
bad = 0
for f in sorted(glob.glob("build3ds/shots/*.ppm")):
    a = np.asarray(Image.open(f).convert("RGB"))
    o, b = int(olive(a).sum()), black8(a)
    flag = o > base_o or b > base_b
    bad += flag
    if flag: print(f, "olive", o, "black8", b)
print("frames", len(glob.glob("build3ds/shots/*.ppm")), "flagged", bad, "base olive", base_o, "base black8", base_b)
sys.exit(1 if bad else 0)
```
Pass: last line `flagged 0`, exit 0. Fail: exit 1, flagged frame names listed (open only those frames, if one by-eye check is needed).

Limits:
- No switch picks a strong attack. This run uses the default Jump. A stronger attack needs menu taps (padscript) or another formation (`battle=<formation>:<stage>`, names in `3ds/stub_include/battle_name_table.h`). Not verified.
- The black count is relative to the idle golden frame `3ds/golden/battle.png`. The olive RGB range is an estimate. Confirm it on one flagged frame before the row closes.
- Shots are 300 frames (about 86 MB in `build3ds/shots`, git-ignored). Delete them after the scan.

Row done when: the battle run exits 0 with `smoke done` in the log, `flagged 0` from the scan, and the `gbi skip` list is saved in the result.

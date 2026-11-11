# Row TOOLB7: a base run keeps its ELF and its crash evidence

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (HOTORD, 2026-11-10)
`bench.py base spot_mac01_det --row HOTORD` at d44903735: run 245748 (cold start after "warm restart failed") ended rc 2
"crash symbols; stopped: matched" (3ds/logs/test_budget.csv 23:27:28); run 246569 (warm restart, same Azahar) passed. The slot 1
emulator log of that Azahar (pid 56624) has 64 UnmappedAccess reads, all at 14 s (2.4 s after its BootGame at 11.7 s), none after the
warm BootGame at 27.7 s: Read32 @ 0x01..0x2D (odd addresses) at PC 0x00100630 (32x), 0x001120AC (24x), 0x001120B4 (6x), 0x0011209C (2x).
The PCs cannot be named: the base ELF is gone, and these PCs lie in the hot group, which moves with hot_order.ld.
## Cause of the lost evidence (oracle code read)
- bench.py base_build_run :1655 copies only pm_3ds.3dsx into the base lane folder, so run_azahar.ps1's addr2line (TOOLB6, ELF of the
  lane folder) has no ELF for a base run.
- bench.py :1663 removes the lane folder (and with it last_crash.txt, TOOLB6) and :1664 the scratch build with its ELF.
## Start
`sh 3ds/tools/wt_new.sh TOOLB7` (build3ds/wt/TOOLB7, branch TOOLB7 from main). Not at the same time as TOOLB5 (both edit bench.py).
## Touch only
3ds/tools/bench.py, base_build_run only.
## Exact edits
1. After `shutil.copy2(src3dsx, lane_dir / "pm_3ds.3dsx")` (:1655) add
   `if src3dsx.with_name("pm_3ds.elf").exists(): shutil.copy2(src3dsx.with_name("pm_3ds.elf"), lane_dir / "pm_3ds.elf")  # TOOLB7: crash symbols of a base run`
2. In the `finally:` at :1662, before `shutil.rmtree(lane_dir, ignore_errors=True)`, add
   `keep = REPO / "build3ds" / "out"  # TOOLB7: a base run's crash evidence outlives the lane folder`
   `if (lane_dir / "last_crash.txt").exists():`
   `    keep.mkdir(parents=True, exist_ok=True)`
   `    shutil.copy2(lane_dir / "last_crash.txt", keep / ("base_%s_last_crash.txt" % short))`
   `    if (lane_dir / "pm_3ds.elf").exists(): shutil.copy2(lane_dir / "pm_3ds.elf", keep / ("base_%s_pm_3ds.elf" % short))`
   `    print("base %s: crash evidence kept in %s" % (short, posix(keep)))`
## Gate
- `python 3ds/tools/bench.py run nd_host --row TOOLB7 --why "TOOLB7 import check"`: PASS (no Azahar start).
- `python -c "import ast,sys; ast.parse(open('3ds/tools/bench.py',encoding='utf-8').read())"`: no error.
The real path is proven by the next base run that ends rc 2 (row HOTORD).
## Stop and end
A failed gate or an edit that does not fit the code: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate results.

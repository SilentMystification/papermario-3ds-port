# Row UB1: two out-of-bounds census accesses (read and write), byte-equal

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read, main 798ee92d9; the old UB1 audit text is in git history, commit 798ee92d9 of this file)
- 3ds/src/pm_gpu.c:3750 `static unsigned spr_nat_n, spr_nat_fb[16];`. The writer (:3796) stores `spr_nat_fb[why & 15]++`, so why 20
  counts at index 4; the reader (:3787-3788, the sprnat census print) reads spr_nat_fb[20]: 16 bytes past the array (UB1 suspect 4).
- 3ds/src/nd_model.c:227 `mf_tot[5]` is written at `mf_tot[sub & 7u]++` (:229): sub 5 to 7 writes past the end (UB1 suspect 8).
- Both are census code; the fixes change no draw. Open, not in this row: the mac_00 layout-dependent sky row (MAC00, not named), the
  unit 1 refresh on a model hit (suspect 1, pm_gpu_model_hit: HC6B's place), suspects 2, 3, 5, 6.
## Start
`sh 3ds/tools/wt_new.sh UB1`; PM_ROW=UB1 PM_LANE=UB1 on every call.
Other rows: WALLSHADE edits pm_gpu.c pm_gpu_spr_build (:3869); HC6B edits pm_gpu.c model key/hit/submit and the nd_model.c native cache.
Do not edit those places. `git merge main` before the completing commit.
## Touch only
3ds/src/pm_gpu.c :3750, :3787-3788, :3796. 3ds/src/nd_model.c :227. No other line.
## Exact edits
1. pm_gpu.c:3750: `spr_nat_fb[16]` -> `spr_nat_fb[32]`.
2. pm_gpu.c:3796: `spr_nat_fb[why & 15]++` -> `spr_nat_fb[why & 31]++` (why 20 now counts at 20, the index the census print reads).
3. nd_model.c:227: `mf_tot[5]` -> `mf_tot[8]` (the writer masks with 7).
## Gate (CLAUDE.md 2)
- `PM_ROW=UB1 PM_LANE=UB1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row UB1 --why "UB1 bounds"`: PASS.
- Byte compare (one list): spot_mac01_det and mdl5_iwa10_det: `bench.py base` each, one `run`, `cmp` each: 6/6. A difference is a
  layout effect (the .bss grows 76 B): stop and hand back the cmp output (it names a layout-dependent scene, MAC00 class).
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.

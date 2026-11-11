# Row HOTORD: regenerate 3ds/ld/hot_order.ld from a fresh fncov census (plan_hotcode.md HC2, HC10)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Why
HC4e4 (f3f548888) left 15 names in hot_order.ld that the map no longer has (hotset_size rc 1: g2_same tex_op g2_note op_settile
gbi_moveword set_fillcolor nd_rec_state tex_op_impl op_setothermode op_settimg op_loadtlut op_settilesize op_loadblock
op_setprimcolor rec_op). hot_order.py (3ds/tools/hot_order.py) writes the file from the fncov logs; a name the normal map does not
have stops it (nothing written). Link order only: no draw change, so the pictures must stay byte-equal.
## Start
`sh 3ds/tools/wt_new.sh HOTORD` (build3ds/wt/HOTORD, branch HOTORD from main). PM_ROW=HOTORD on every call. Normal lane: PM_LANE=HOTORD
(build3ds/lane-HOTORD). Census lane: PM_LANE=HOTORD PM_FNCOV=1 (build.sh writes build3ds/lane-HOTORD_fncov; bench calls use --lane HOTORD_fncov).
No azahar.exe open outside build3ds\azslots during the Azahar steps (TOOLB4 runs at the same time: its one run is fine, it is a slot run).
## Touch only
3ds/ld/hot_order.ld (written by hot_order.py, never by hand). No other file.
## Steps (commit after each passed step)
1. `PM_ROW=HOTORD PM_LANE=HOTORD sh 3ds/build.sh > build3ds/out/build_n1.txt 2>&1`: rc 0, memcheck OK.
2. `PM_ROW=HOTORD PM_LANE=HOTORD PM_FNCOV=1 sh 3ds/build.sh > build3ds/out/build_f.txt 2>&1`: rc 0.
3. Census, census lane, one start each (copy the log right after each run, before the next run):
   `python 3ds/tools/bench.py run fncov_mac01_det --row HOTORD --lane HOTORD_fncov --why "HOTORD census mac_01"`, then
   `cp build3ds/azt/logs/fncov_mac01_det.txt build3ds/out/`. The same for fncov_kmr10_det and fncov_mac00_det.
   Each: LOG CHECKS PASS (2 of 2) (warp line, "fncov f<n> done").
4. `PM_LANE=HOTORD python 3ds/tools/hot_order.py build3ds/out/fncov_mac01_det.txt build3ds/out/fncov_kmr10_det.txt build3ds/out/fncov_mac00_det.txt > build3ds/out/hot_order.txt 2>&1`:
   rc 0 and 3ds/ld/hot_order.ld changed. Commit "HOTORD: hot_order.ld from fncov mac_01, kmr_10, mac_00".
5. `PM_ROW=HOTORD PM_LANE=HOTORD sh 3ds/build.sh > build3ds/out/build_n2.txt 2>&1`: rc 0 (it rebuilds: 3ds/ld is in the build key).
## Gates (CLAUDE.md 2: nd_host and byte compare, no picture)
- memcheck line of build_n2.txt: OK. Write it in the commit.
- `PM_LANE=HOTORD python 3ds/tools/bench.py run hotset_size --row HOTORD --lane HOTORD --why "HOTORD stale names"`: rc 0, 0 stale names.
  Write m1_union and the M2 linker bytes in the commit.
- `python 3ds/tools/bench.py run nd_host --row HOTORD --why "HOTORD host test"`: PASS.
- Byte compare, 2 scenes (these benches record under their own name; not spot_mac00_det, see row TOOLB5):
  for spot_mac01_det and spot_mac00_det_pic: `python 3ds/tools/bench.py base <bench> --row HOTORD` (the merge-base; "stored" or
  "two runs equal, stored"), `python 3ds/tools/bench.py run <bench> --row HOTORD --lane HOTORD --why "HOTORD byte gate"`,
  `python 3ds/tools/bench.py cmp <bench> --row HOTORD`: exit 0, 6/6 byte-equal.
## Azahar starts
3 census + 2 gate runs = 5 counted (budget 8). The 2 bases are 4 more starts, not counted (base runs), 0 when already stored.
## Stop and end
hot_order.py names a missing function, a STALE BUILD refusal, a failed gate, or a tool fault: commit and hand back the exact output
(CLAUDE.md 4-5). At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.

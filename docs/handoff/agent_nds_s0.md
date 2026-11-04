# agent_nds_s0: S0 NDTEST handoff (2026-11-04)

## State
- Branch nds1. Baseline build 2020 at commit 42f6fac7e (build3ds/pm_3ds.3dsx). Status: BASELINE BUILD READY 2020 42f6fac7e. Not pushed. Do not rebuild build3ds/ until BASELINE PUSHED is in 3ds/logs/perf/status_NDS_S0.txt.
- Lane build with the budget line: build3ds/lane-budget/pm_3ds.3dsx (commit 799307404). Not the baseline.
- S0-1 PASS (ndref_toad: 64 draws, file written). S0-3 PASS except the nd_core check (FAIL by design, row S1-2). S0-4 PASS on the fixture (88 bytes, 19 instructions). S0-5 step 1 PASS (67 shared positions), step 2 FAIL by design (nd_mtx, row S1-4). S0-6 PASS (mac01t_prof stage line). S0-8 PASS (budget line, lane budget).
- S0-2 INCOMPLETE. Files in 3ds/tests/ndref/: yard, pause, battle, text, title (frame 800, game mode 16, not the title), splash (frame 412). All pass nd_ref_dump.py --check, and the draw counts equal the census batch counts of the same run. Missing: ndref_toad.bin (file overwritten by later runs), ndref_stereo.bin (not run). Row budget spent (8 of 8, the ninth start was refused, rc 9).
- S0-7 baseline is ready. The orchestrator pushes build3ds/pm_3ds.3dsx with --no-build.

## Commits (S0)
- e95b21495 S0-1 NDREF1 hook and ndref_toad
- 42f6fac7e S0-6 stage line and GPU busy probe, S0-2 scenarios and dumper
- 4175bfdd5 S0-2 partial reference files
- 7168a7935 S0-3 host harness
- d608bbede S0-4 size test
- d83b89eb8 S0-5 seam test step 1
- 799307404 S0-8 frame budget doc and budget line
- 27571b43e docs: Q answers, S0-7 and S0-8 rows

## Next
1. Coordinator: run `run_azahar.ps1 -ResetBudget S0-2`. Then run ndref_toad and ndref_stereo (-Try, -Row S0-2), copy ndref.bin to 3ds/tests/ndref/<scenario>.bin after each run, run nd_ref_dump.py --check, compare the draw count with the cen summary line, commit.
2. S0-2 FORMAT notes: stereo frames hold both eyes in one file. Title scene is boot mode 16. The title needs a START press (not tested).
3. S1 gaps found by S0-3: the decoder does not map cull, blend, depth, alpha, TEV or vertices yet. The compare checks texture words and triangle counts only.
4. S0-6: nd and leg are 0 until S1 (pm_pf_nd_add in pm_phase.c). gb is submit to sync, so it includes CPU time before the sync (20 to 21 ms in Azahar). It is an upper bound of GPU busy.
5. S0-8: the logic and list build fields are not measured in the prof line. Use fsplit lines of a console run.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_ROW=<row> sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from the user copy in the scratchpad (md5 437b90efe5f32b82394ce2ece094210e). Never commit that file.
- Budget lane: `PM_LANE=budget PM_ROW=S0-8 sh 3ds/build.sh` (writes build3ds/lane-budget/).
- Azahar: `PM_ROW=S0-2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 ndref_stereo -Row S0-2 -Try`. Write the justification line to 3ds/logs/perf/status_NDS_S0.txt first.
- Dump check: `python 3ds/tools/nd_ref_dump.py --check 3ds/tests/ndref/*.bin`
- Host tests (docker pm3ds-host:1): `sh 3ds/tools/nd_host_test.sh`
- Size test (docker pm3ds-build:1): `sh 3ds/tools/nd_size_test.sh`
- N64 same check: `sh 3ds/tools/nd_n64_same.sh src/npc.c`

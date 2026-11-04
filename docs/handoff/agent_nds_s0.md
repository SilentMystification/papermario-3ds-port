# agent_nds_s0: S0 NDTEST handoff (2026-11-04)

## State
- Branch nds1. Baseline build 2020 at commit 42f6fac7e, pushed to O3DS XL (BASELINE PUSHED, log build3ds/hw_live_log_131.txt). Baseline numbers in docs/handoff/frame_budget.md section 3.
- Lane build with the budget line: build3ds/lane-budget/pm_3ds.3dsx (commit 799307404). Not the baseline.
- S0-1 PASS (ndref_toad: 64 draws, file written). S0-3 PASS except the nd_core check (FAIL by design, row S1-2). S0-4 PASS on the fixture (88 bytes, 19 instructions). S0-5 step 1 PASS (67 shared positions), step 2 FAIL by design (nd_mtx, row S1-4). S0-6 PASS (mac01t_prof stage line). S0-8 PASS (budget line, lane budget).
- S0-2 PASS. All eight files in 3ds/tests/ndref/ pass nd_ref_dump.py --check. Draw counts equal the census batch counts (toad 64 and 810 triangles, stereo 88). Title file is boot mode 16 at frame 800, not the title.
- S0-7 baseline pushed by the orchestrator (build 2020).

## Commits (S0)
- e95b21495 S0-1 NDREF1 hook and ndref_toad
- 42f6fac7e S0-6 stage line and GPU busy probe, S0-2 scenarios and dumper
- 4175bfdd5 S0-2 partial reference files
- 7168a7935 S0-3 host harness
- d608bbede S0-4 size test
- d83b89eb8 S0-5 seam test step 1
- 799307404 S0-8 frame budget doc and budget line
- 27571b43e docs: Q answers, S0-7 and S0-8 rows
- 9d628a8b3 handoff (first version)
- 6401d08e4 S0-2 toad and stereo files
- 7b9f465e3 frame budget console baseline

## Next
1. S1 gaps found by S0-3: the decoder does not map cull, blend, depth, alpha, TEV or vertices yet. The compare checks texture words and triangle counts only.
2. S0-6: nd and leg are 0 until S1 (pm_pf_nd_add in pm_phase.c). gb is submit to sync, so it includes CPU time before the sync. It is an upper bound of GPU busy.
3. S0-8: logic and list build are not in the prof line. Use fsplit lines of a console run.
4. Coordinator quoted baseline ranges differ from the pushed log. The log values are in frame_budget.md section 3.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_ROW=<row> sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from the user copy in the scratchpad (md5 437b90efe5f32b82394ce2ece094210e). Never commit that file.
- Budget lane: `PM_LANE=budget PM_ROW=S0-8 sh 3ds/build.sh` (writes build3ds/lane-budget/).
- Azahar (budget reset by the orchestrator, 2 of 8 used after reset): `PM_ROW=S0-2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 ndref_stereo -Row S0-2 -Try`. Write the justification line to 3ds/logs/perf/status_NDS_S0.txt first.
- Dump check: `python 3ds/tools/nd_ref_dump.py --check 3ds/tests/ndref/*.bin`
- Host tests (docker pm3ds-host:1): `sh 3ds/tools/nd_host_test.sh`
- Size test (docker pm3ds-build:1): `sh 3ds/tools/nd_size_test.sh`
- N64 same check: `sh 3ds/tools/nd_n64_same.sh src/npc.c`

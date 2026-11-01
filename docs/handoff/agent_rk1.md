# agent_rk1: RK1 (record capture with the matrix bump fixed, RC1 reapplied)

Plan: docs/handoff/plan_record_reuse.md row RK1. Base: HEAD 8850f3900 (source equal to 14456994d).
Status: gate passed. Committed on main (see git log for the hash). Map_restore_table.c not committed.

## Changed files
- 3ds/include/pm_gpu.h, 3ds/src/pm_gpu.c, 3ds/src/pm_gbi.c, src/model.c: RC1 patch (scratchpad rc1_patch.diff, applied with git apply).
- src/model.c 2852: PM_REC_BUMP only when MODEL_FLAG_IGNORE_MATRIX is set (the flag clear stays the same).
- 3ds/src/pm_gbi.c rec_try: counts rb (recapture of a slot with an earlier key) and ls (that slot). rec line adds `rb <n> ls <slot>`.
- 3ds/tools/azt_scenarios.json: scenario mac01t_rec (until `rec f6\d\d `, shots 500 to 505).
- docs/handoff/test_catalog.md: row mac01t_rec after mac01t_cap1.

## Build
- `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh` (exit 0, no warning in changed lines), restore of map_restore_table.c from .bak.

## Gate (PM_ROW=RK1H for HEAD, PM_ROW=RK1 for the build; status in 3ds/logs/perf/status_RK1.txt)
| Start | Build | Scenario | Result |
|---|---|---|---|
| 1 | HEAD (lane rc1h) | mac01t_cap1 (same debug string; mac01t_rec needs a rec line, HEAD prints none) | top shots 0000_1_2, 0001_1_2 saved |
| 2 | HEAD (lane rc1h) | yard | top shot 0000_0_11 saved |
| 3 | RK1 | mac01t_rec | rec f307 cap 303 build 79 ref 0 emp 224 wbad 0 kb 61 rb 0; rec f614 cap 0 build 0 wbad 0 rb 0. Shots 0000_1_2 and 0001_1_2 byte-equal to start 1 |
| 4 | RK1 | yard | rec f348 cap 48 build 9 ref 0 emp 39 wbad 0 kb 6 rb 0. Top shot 0000_0_11 byte-equal to start 2 |

- Window 2 cap 0 (gate: under 5). wbad 0 in both scenes. Top shots equal.
- Bottom shots differ between runs of the same build (57 of 61 differ between two HEAD yard runs). Only top shots are gated.

## Open items
- Window 1 of mac01t_rec: cap 303 in one 300-frame window (one capture per frame), emp 224. Not a gate item. Window 2 is 0. Cause not read. A fresh investigation names the site if this matters for RK2.
- The yard run ends at scene f300. Its rec line is at f348 (printed before the run stopped).
- Picture of mac01t_rec not read by eye in this row. Equal to the HEAD shot, which was read in CAP1.

## Commands
- Start: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scene> -Row <row> [-Build rc1h] [-Try]`.
- Rec line: `grep "rec f" build3ds/last_log.txt`.
- Never commit 3ds/src/map_restore_table.c.

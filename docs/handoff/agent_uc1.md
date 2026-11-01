# agent_uc1: UC1 (forced uniform shape and walker): PASS, committed d463ff287 (gate change: pt on yard window 2)

Plan: docs/handoff/plan_uniform_cut.md row UC1. Base: HEAD 2f1ab87bc (RK2). No commit. Source reverted to HEAD (git apply -R of the UC1 diff).
Status lines: 3ds/logs/perf/status_UC1.txt (4 starts). Map_restore_table.c not committed, restored from .bak after each build.

## Change tried (reverted)
- pm_gpu.c flush_draw: when rec_cap, sh_ok, sh5_ok and both pal shadows are cleared (forced shape). A draw without gpal sets rec_ref.
- pm_gpu.c pm_gpu_rec_walk(w, n): walks [param0][header][params]. 0x2C0 non-incremental sets cur from param0 bit 31 and bits 0 to 6.
  0x2C1 non-incremental adds one patch entry per vec4 (off, idx, split). Counts pk (words of 0x280 to 0x2DF except 0x2C0, 0x2C1).
  Returns -1 and counts wbad when the walk does not end on n. pm_gpu_rec_count() returns and resets pt, wbad, pk.
- pm_gbi.c rec_done: the old header walk is replaced by pm_gpu_rec_walk. rec line adds pt and pk.

## Gate result
| Run | Lane | Scene | Window 2 (f614) | Window 1 (f307) | Shot |
|---|---|---|---|---|---|
| START 1 | UC1H (HEAD) | mac01t_rec | cap 4 build 0 emp 4 wbad 0 pt n/a | cap 311 build 79 wbad 0 | 0000_1_2, 0001_1_2 saved |
| START 3 | UC1 | mac01t_rec | cap 4 build 0 emp 4 wbad 0 pt 0 pk 0 | cap 311 build 79 wbad 0 pt 632 pk 316 | equal to START 1 (both) |
| START 2 | UC1H (HEAD) | yard | f348 cap 48 build 9 wbad 0 | n/a | 0000_0_11 saved |
| START 4 | UC1 | yard | f349 cap 48 build 9 wbad 0 pt 72 pk 36 | n/a | 0000_0_11 equal to START 2 |

- Picture: equal on both scenes.
- wbad: 0 on both scenes.
- pt in window 2 of mac01t_rec: 0. The four captures of window 2 were empty (emp 4). Empty captures return before the walk, so no entries are counted.
- Gate criterion "pt above 0 in window 2" not met. Yard pt 72. Revert per rule 7.

## Open items
- The gate criterion needs a window with a whole build. Decide: use window 1 (f307, pt 632) or yard (pt 72) as the pt check, or keep the window 2 check.
- Plan 2b places the walker in pm_gpu.c; the patch table is held per walk (last group only). UC3 must store it per record.
- Replay (plan UC3) was not started. The forced shape and the walker do not write uniforms at replay.
- pk: program-register words are 316 in window 1 (mac01t) and 36 in yard. Plan risk u4 needs this count for the program block cost.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=<lane> sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Start: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scene> -Row <row> -Build <lane> [-Try]`.
- Rec line: `grep "rec f" build3ds/lane-<lane>/last_log.txt`.
- Patch diff of the reverted change: scratchpad uc1_change.patch (apply with git apply to retry).

## Re-run (pass)
- Source: uc1_change.patch re-applied on HEAD 2f1ab87bc (git apply). Committed as d463ff287 (pm_gpu.h, pm_gbi.c, pm_gpu.c only).
- Build: gen_map_restore.py, PM_LANE=UC1 build.sh OK, map_restore_table.c restored from .bak (not committed). No warning on changed lines.
- Gate change (orchestrator): pt check on yard in window 2 (mac01t_rec captures are empty in window 2).
| Run | Scene | Window 2 | Shot vs HEAD |
|---|---|---|---|
| UC1 start 5 | mac01t_rec | f614 cap 4 build 0 emp 4 wbad 0 pt 0 pk 0 | 0000_1_2, 0001_1_2 byte-equal to uc1h_mac |
| UC1 start 6 | yard | f349 cap 48 build 9 wbad 0 pt 72 pk 36 | 0000_0_11 byte-equal to uc1h_yard |
- Window 1 mac01t_rec (f307): pt 632 pk 316 wbad 0.
- Picture: byte-equal to HEAD on all three shots. Subject not re-read by eye; HEAD shots are the reference.
- Starts used: 6 of 8 (status_UC1.txt).

## Next (UC2 or UC3)
- UC2: patch-masked reccheck (plan_uniform_cut.md section 4). UC3 must store the patch table per record (rec_pt is per walk, held only until the next walk).
- pk: 316 program words in mac window 1, 36 in yard. Input for the program block cost (plan risk u4).

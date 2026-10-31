# agent_dlm: rows EPO1, SORT1, MTXM (plan_dl_rows.md rows 4, 5, 9) - result: all three DROPPED, no code commit

## State
- No source change is committed. 3ds/src/pm_gbi.c and src/model.c match HEAD 1cd3ae2e1 (git diff empty).
- 3ds/src/map_restore_table.c restored from a backup after each build (never committed).
- Base for all pictures: HEAD 1cd3ae2e1, lane epc (HEAD plus one counter, no behaviour change).

## Results
| Row | Quantity (mac01t_cap1, sp0c window f483, per 300 frames) | Gate | Result | Starts |
|---|---|---|---|---|
| EPO1 epoch bump only on change | ep (gv_epoch_enter calls): base 12724 and 12630; mod 12684 | down 20 % | 0.3 % drop. DROPPED | 4 |
| SORT1 task order memo | rs (memo hits): 28 | 300 (every frame) | DROPPED | 1 |
| MTXM matrix unpack memo | mx: 41739 unpacks, 771 hits (2.6 per frame) | 90 per frame | DROPPED | 1 |

Pictures:
- HEAD against HEAD, mac01t_cap1: both shots (0000_1_2, 0001_1_2) byte-equal in two runs.
- EPO1 mod: shot 0000 equal to base, shot 0001 differs. Not traced (row dropped). A stale-state effect is possible.
- SORT1 mod: both shots equal to base.
- MTXM mod: both shots equal to base.
- yard was not run for any row (no row passed its quantity gate).

## Trace (from code, before each edit)
- EPO1: the epoch bump sits in the walker (pm_gbi.c, generic op bump), in mg_run, and in gbi_run_body. gv_epoch_enter is called on the first triangle after each bump. Removing the bump only for skipped tb_run lists and for rm_apply lists with no field change removed 0.3 % of enters. Most bumps come from other ops.
- SORT1: the three exchange sorts in execute_render_tasks (src/model.c) depend only on the task dist values in index order. A memo on the dist sequence is exact. The dists change in about 90 % of frames in the still window (28 of 300 hits), so the memo does not repeat.
- MTXM: load_mtx (pm_gbi.c) unpacks each G_MTX matrix with 16 divides. The memo was a 16-entry direct map keyed by address. About 139 unpacks per frame use more addresses than 16 slots, so the hit count is 2.6 per frame. A larger table (about 105 entries of 132 bytes) would break the L1D rule in hw_guidelines.md rule 2 and needs a design decision. Not tested.

## Open items
- The DUP0 census was not built. Each row added only its own counter, and each counter was reverted with its row.
- The three rows are dropped. No row of this group is open.
- The MTXM hit count depends on the table size. A test with more entries would be a new design. Ask first (plan rule: no full runs without user approval).
- EPO1 shot 0001 difference is not explained.

## Starts
- EPO1: 4 of 8 (epc x2 mac01t_cap1, epc yard, epm mac01t_cap1).
- SORT1: 1 of 8 (srtm mac01t_cap1).
- MTXM: 1 of 8 (mxm mac01t_cap1).

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=<row> sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c from a copy.
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row <row> -Build <lane> -Try`.
- Quantity: `grep sp0c` in %APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt.

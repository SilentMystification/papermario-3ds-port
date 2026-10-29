# Sweep 420: texture cache over the shipped maps (HEAD vs tex_inv2 reverted)

## Scope and list
- Shipped list: 411 maps = 420 maps of 3ds/maps.cmake minus 9 test maps (tst_01, tst_02, tst_03, tst_04, tst_10, tst_11, tst_12, tst_13, tst_20). The filter is pm_warp_is_test_map (prefix `tst_`).
- Kept: end_*, gv_01, mgm_*, and all other areas. Not checked against the decomp area table beyond the tst_ prefix.
- List file: tour_s420.txt in the Azahar SD folder (grep -v ^tst_ of tour_all.txt).

## Runs (PM_ROW=sweep420, budget 4 of 8 starts used)
| run | build | list | result |
|---|---|---|---|
| 1 | HEAD + log hunk (lane-texmax) | 420 maps | crash at tst_12 (map 418), assert gEffectInstances, watchdog exit |
| 2 | (baseline) | 420 maps | killed at start (test maps in list) |
| 3 | HEAD + log hunk (lane-texmax) | 411 maps | tour: done, 411 maps, no assert |
| 4 | tex_inv2 reverted (lane-sweepbase) | 411 maps | tour: done, 411 maps, no assert |

- Run flags: -Milestone -Force -Timeout 1800 -Until "tour: done", debug "tour=@tour_s420.txt shots shotwin=999999:1 shotmode=4:21 texmax".
- Logging added (pm_tex.c pm_tex_map_state, pm_mem.c texmap state line): `texmap state: slots N used KB evict cum N tex fail cum N delta N vramfree min N KB`, printed at each map load. Hit and miss come from the existing texmap line.
- Binary note: the run binaries are built from the tree before commit 19b8d4a5a. They lack the tex_inv3 timers. The timers do not change cache behaviour.
- The tour is one pass. No map repeats, so "pass-2" misses do not exist in this sweep. Compare the single-pass miss totals below.

## Result: first texture failure
- HEAD: kmr_30 (tour entry 153). Delta 420. Cache before the failure: 27 slots, 1288 KB of 1536 KB. Cache line before it: `texcache VRAM full 256x176` (log #12912). vramfree min 2770 KB (not VRAM exhaustion). Shot: SD shots/0153_0_16.ppm. Shot not viewed.
- Baseline: flo_23 (entry 52). Delta 242. Second fault kmr_06 (entry 142), delta 150. Cache before first failure: 55 slots, 324 KB.
- HEAD has no failure after kmr_30. Cumulative failures: HEAD 420, baseline 392.

## Per-map table (selected rows; full tables: scratchpad run3_head411_table.txt, run4_base411_table.txt)
Columns per run: slots, cache KB used, miss (this map), evict cum, tex fail cum, vramfree min KB.

| entry | map | HEAD slots/KB/miss/evict/fail/vmin | base slots/KB/miss/evict/fail/vmin |
|---|---|---|---|
| 1 | arn_02 | 61/405/62/0/0/3858 | 60/397/74/0/0/3858 |
| 50 | flo_21 | 257/1518/49/413/0/2834 | 230/1483/49/149/0/3858 |
| 52 | flo_23 | 120/895/19/574/0/2834 | 55/324/71/348/242/3858 |
| 100 | jan_11 | 287/1523/52/1815/0/2786 | 256/1519/98/933/242/2786 |
| 142 | kmr_06 | 66/510/39/2800/0/2770 | 58/352/111/1480/392/2770 |
| 153 | kmr_30 | 27/1288/45/3106/420/2770 | 28/1416/28/1722/392/2770 |
| 225 | mac_04 | 223/1301/20/6331/420/2770 | 97/750/67/4105/392/2770 |
| 300 | pra_19 | 249/1522/31/7424/420/2770 | 175/1048/39/4536/392/2770 |
| 350 | sbk_30 | 206/1486/90/7978/420/2770 | 207/1510/143/4876/392/2770 |
| 411 | trd_10 | 290/1463/19/8741/420/2770 | 263/1487/61/5467/392/2770 |

Totals over 411 maps (single pass):
- Miss total: HEAD 15418, baseline 26149.
- Fault maps (delta above 0): HEAD 1 (kmr_30), baseline 2 (flo_23, kmr_06).
- Evictions cumulative at end: HEAD 8741, baseline 5467.

## Shots
- Shot written for every map (shotmode=4:21, last frame per burst). Files: SD folder shots/<entry>_<area>_<map>.ppm, 411 per run.
- Shot of the first HEAD failure: shots/0153_0_16.ppm. Shot of the first baseline failure: shots/0052_*.ppm (flo_23). Pictures not checked by eye.
- Run 3 shots copy: scratchpad run3_shots. Run 4 shots copy: scratchpad run4_shots.

## Open items
1. Crash in run 1 at tst_12: effect pool full (assert i < ARRAY_COUNT(gEffectInstances)). Test map, excluded. Not analysed.
2. The lower table miss total on HEAD is in one pass. Two-pass comparison (revisits) is not run.
3. Cause of 1088 KB vramfree difference at start (HEAD 2834 vs baseline 3858 on map 25): not found.

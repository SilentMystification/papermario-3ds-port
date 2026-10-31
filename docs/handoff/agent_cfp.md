# agent_cfp: rows DBGF, HOOK1, LAY2, TST1, MCEN (plan_icache_rows.md rows 4, 5, 6, 7, 8)

Branch: main. Pure moves and layout. The hot path keeps the same operations in the same order. No change to what is drawn.

## Result
| Row | Result | Commit | Map bytes before -> after | Starts |
|---|---|---|---|---|
| DBGF (debug switches read once at boot) | DROPPED, map gate | none | sum of 7 bodies (tex_op, flush_draw, gbi_run_body, draw_tri, sync_combine, sync_combine_work, mg_run): 28200 -> 27992 (gate: 27000 or less) | 0 |
| HOOK1 (walker hooks in one cold call) | DROPPED, not built | none | gbi_run_body 6828 (gate 5600), gv_tri 2516 (gate 2000). Not measured by a build. | 0 |
| LAY2 (hot list complete, call order) | COMMITTED | 9bb97f3fb | hot block 0x10c3cc to 0x10f908 (18 names), span 57.7 KB -> 63.8 KB. No code bytes change. | 5 |
| TST1 (tile and combine state in one struct) | COMMITTED | 88d689bcd | pm_gbi.o .data.pmhot 0x304 B (struct tst1, aligned 32). No code bytes change. | 2 |
| MCEN (appendGfx_model census out of line) | DROPPED, map gate | none | appendGfx_model 5284 -> 4844 (gate: 4500 or less) | 0 |

## Pictures (byte compare, top shots)
- HEAD against HEAD (lane h27): mac01t_cap1 0000_1_2 and 0001_1_2 equal (2 runs).
- LAY2 (lane lay2) against HEAD: mac01t_cap1 2 of 2 equal; yard 0000_0_11 equal.
- TST1 (lane tst1) against HEAD: mac01t_cap1 2 of 2 equal; yard 0000_0_11 equal.
- The HEAD shots are the h27 runs. Shot copies: scratchpad shots folders (h27_mac_a, h27_mac_b, h27_yard, lay2_*, tst1_*).

## Why the drops
- DBGF: the lazy `static int v` tests inline to small code. Moving the reads to boot saved 208 B, not 1200 B. The plan estimate (1312 B) counted the pm_debug_has calls, which are small once inlined.
- MCEN: moving the census blocks saved 440 B. The plan estimate (about 840 B) did not match the map.
- HOOK1: the plan's listed sites (about 970 B in gbi_run_body, 230 B in gv_tri by count) cannot reach the gate (gbi_run_body 5600 B needs 1.2 KB, gv_tri 2000 B needs 0.5 KB). The row was not built. A build would confirm it.

## Open items
- Plan rows OS1 and FLH/TEXH are already on main (FLH 589021dd3, TEXH 08b1848ed). No pmu or console push was run (rule 6: the epic is not complete). The console decides LAY2 and TST1.
- TST1 has no map gate in the plan. Its check is the .data.pmhot placement only.
- The HOOK1 build was not run. Recheck the sums with mapsz-style sums if the plan is revisited.
- Working tree: src/model.c shows as modified in git status with a matching blob hash (stale index). The user file 3ds/src/map_restore_table.c is restored after each build and never committed.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=<row> sh 3ds/build.sh`; then copy the user file back (saved copy in the session scratchpad).
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row <row> -Build <lane> -Try` and `... yard -Row <row> -Build <lane>`. Shots: build3ds/azt/shots/<scenario>, overwritten by the next run. Copy them at once.
- Map sizes: a script that sums `.text.<name>` lines of build3ds/lane-<lane>/pm_3ds.map (the size is on the next line when the name is long).

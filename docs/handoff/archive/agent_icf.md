# agent_icf: epic ICF (plan_icache_rows rows FLH and TEXH)

Branch: main. Pure code moves. The hot path keeps the same operations in the same order.

## Commits
- 589021dd3 FLH (pm_gpu.c): CPU-vertex tail to flush_draw_cpu (noinline); light table to flush_light_tab; pcomp recompute to pcomp_calc; nozt test to nozt_on; off-active scissor to scissor_off; off-active blend to blend_off; fl debug to flush_fl_dbg (cold).
- 08b1848ed TEXH (pm_gbi.c): tile 1 block to tex_tile1 (noinline); shaded sprite to tex_shaded0 (cold); S25 negative window to tex_win_s25 (cold); V20 palette loop to tlut_v20 (noinline); GP_SET body to gp_note (cold); dead imglog block deleted.

## Map (build3ds/lane-<name>/pm_3ds.map; base = lane flhbase, HEAD 41f50ffea)
| Function | Before | After |
|---|---|---|
| flush_draw | 13712 B | 11192 B |
| flush_draw_cpu (new) | - | 1512 B |
| flush_light_tab / flush_fl_dbg / pcomp_calc / scissor_off / blend_off / nozt_on | - | 352 / 316 / 192 / 160 / 100 / 48 B |
| tex_op | 7848 B | 5956 B |
| tex_tile1.constprop.0 (new) | - | 1136 B |
| tex_shaded0 / tlut_v20 / tex_win_s25 / gp_note (new) | - | 304 / 248 / 140 / 48 B |

Plan targets: flush_draw 10.2 KB (not reached, 11.2 KB); tex_op 5.7 KB (not reached, 5.96 KB). The plan's DC_T(1)/DC_T(2) item was skipped: pm_ticks is an svc call, the sample body is about 20 B.

## Pictures (shot_cmp by eye not done; byte compare with cmp)
- HEAD 41f50ffea, mac01t_cap1: 2 runs, shots 0000_1_2 and 0001_1_2 byte-equal (HEAD against HEAD).
- FLH vs HEAD: mac01t_cap1 2 of 2 equal; yard top 0000_0_11 equal; pause_menu top 0000_1_1 equal.
- TEXH vs HEAD (HEAD shots from the flhbase runs, same binary as 41f50ffea): mac01t_cap1 2 of 2 equal; yard top equal.
- The script does not read the pictures. The subject was not checked by eye. Log checks: FLH mac 6 of 6, yard 3 of 3, pause 7 of 7; TEXH mac 6 of 6, yard 3 of 3.

## Azahar budget (starts per row)
- FLH: 7 of 8 (mac01t_cap1 HEAD 2, FLH 1; yard HEAD 1, FLH 1; pause_menu HEAD 1, FLH 1).
- TEXH: 2 of 8 (mac01t_cap1 1, yard 1). HEAD shots reused from the FLH runs.

## Open items
- Console: no push (rule 6, epic not complete). The pmu B share decides the epic (plan_icache_rows section 4). Step 0 (phase-off baseline) not run.
- Plan rows OS1, DBGF, HOOK1, LAY2, TST1, MCEN not started.
- The status files 3ds/logs/perf/status_FLH.txt and status_TEXH.txt hold the justification lines.
- map_restore_table.c: restored after each build. Not committed.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, then `PM_LANE=<lane> PM_ROW=<row> sh 3ds/build.sh`, then copy map_restore_table.c back from a copy taken first.
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane> -Try`. Shots: build3ds/azt/shots/<scenario>, overwritten by the next run of that scenario. Copy them at once.
- Map size: grep -A1 "^ \.text\.<name>" build3ds/lane-<name>/pm_3ds.map (the size is on the next line when the name is long).

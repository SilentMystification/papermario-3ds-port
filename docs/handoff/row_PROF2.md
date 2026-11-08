# Row PROF2: profiling with minimal cost to the game (oracle plan 2026-11-08, nds1 744012867)

## Goal
User: "ensure that the performance profiling is done optimally and has minimal impact to game performance". Targets: a clean frame reads no prof tick and makes no svc (csvc 0); the sampled-frame cost (oh, amort) is small and printed; no census line and no double math on the game thread in prof mode; prof=off adds no more than one predicted branch per op in the walker. Phase mode (`phase`) keeps all its timers. One Opus agent, no subagents.

## Start state
Worktree build3ds/wt/prof2, branch prof2 from nds1 (current). Copy assets/us and ver/us/build into it. BT1 (pm_warp.c) and POND (draw path) run in parallel: merge nds1 when told.

## Steps (premise check first; each replaced path deleted in the same commit)
1. SELF-CHECK. pm_pf_frame keeps the pm_svc_n delta (pm_main.c:140) per clean and per sampled frame; the main prof line prints `csvc <clean> ssvc <sampled>` next to oh and amort. Target csvc 0.
2. PHASE-ONLY SITES OFF IN PROF. These test pm_ph_cen, not pm_ph_on: tex_op ticks (nd_tex_set.c:531-535, 558, 577), flush_impl timed path and pm_gbi_flush_class/pm_gbi_census_close (pm_gpu.c:625, 3337-3343), kick timer (pm_gpu.c:607-609), DC1 sampling (pm_gpu.c:3338), sync_combine timer (nd_comb.c:535-538), sp0 split (nd_tex_set.c:73-75), gv timer (nd_geom.c:286, 367; nd_int.h gp_on() -> pm_ph_cen), pm_tex_load timer (pm_tex.c:1257-1271), pm_tex flush timer (pm_tex.c:571-573), sp0 counters (nd_shape.c:502, 507). Check: follow each sum to its reader; phase/opsplit-only readers move. Keep on pm_ph_on: PH_GBI/TEX/GPU, class timers, gb_sub (pm_gpu.c:2190), pm_pf_nd_add (pm_nusys.c:98-100).
3. CENSUS LINES OFF IN PROF. ndn_log (pm_gpu.c ~2813-2837), ent_log (nd_ent.c:280-295), mdlw (nd_model.c:236-240), spr_nat_log, rect_log, wide_log, nrawwhy: print only under pm_ph_cen or a `census` word, via pm_log_w, not pm_log_prof_line. Counters stay. Model cache and entity log calls run once per frame at frame end, not per record. Check: grep pm_log_prof_line( outside pm_phase.c; prof_by_scene.py parses only `prof ` lines.
4. INTEGER REPORT. pf_report, stage, cls and budget lines carry integers (ms*100 as %u.%02u from u64 ticks, TICKS_MS 268112, one divide per window, no double). pf_where passes ints (%d positions) and the static map name pointer, no local snprintf. Lines under about 200 B. Check lf_scan/lf_copy in pm_log.c first. Gate extra: ingest of a tour10_det log parses every field.
5. WALK DEBUG WORD. One `unsigned pm_walk_dbg` (bits cen, fine, c2, ph, orb), loaded into a local once at nd_walk_run entry (nd_walk.c:116-341); each debug test is `if (__builtin_expect(dbg, 0)) walk_dbg_op(...)`, a cold noinline function (gv_streak, gp, os_op_hook, mdc, pk_rec, c2 calls). NdtT a u32 start tick, only inside the PH branch, 4-deep tick stack in pm_gpu.c for the exclusive nesting. n_ops counts only under dbg. Report nm -S of nd_walk_run and the hot-span total (3dsx.ld.in).
6. SMALL. pf_scene_note runs on scene events (pm_ph_scene_map/_mode) with one flag test per frame. Then the Azahar self-check pair: spot_mac01_det with prof=30 against prof off (pacestat work ms; add it if missing), both recorded with bench.py. Direction only: an Azahar result does not drop or change a fix (user rule).

## Done criteria
Every step: build, nd_host PASS, byte-equal to the previous step on spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015; tour10_det prints 10 windows, one per map, each with its `prof ... tour` line. Last: the user pushes spot_mac01_prof (prof=30) to the O3DS XL. Pass there: csvc 0, oh and amort printed, no census lines, ft/wk not above the PERF1 run.
Affected scenarios (no others): the 4 byte gates, tour10_det, the self-check pair.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=prof2 PM_ROW=prof2 on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=prof2 PM_LANE=prof2 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=prof2 PM_LANE=prof2 python 3ds/tools/bench.py run <bench> --row prof2 --lane prof2 --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget prof2

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "prof2 ready <hash>".

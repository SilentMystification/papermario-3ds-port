# Row PERF2: console frame cost after PERF1 (oracle plan 2026-11-08, nds1 98e76eff1)

## Goal
Console O3DS XL (build3ds/hw_live_log_131.txt, PERF1 build, prof=30): mac_05 wk 36 ms, mac_04 36-39 ms, mac_01 spot 23 ms. Misses at 40 cycles each (design_native_draw.md:128-133) are about 59% of the mac_05 frame (80k I-miss, 64k D-read miss per clean frame). Classes: sub about 85 us per draw; ent 7-11 ms in towns; spr 4.5 ms; rest 11 ms (not split). Hot span 25,448 B against the 16 KB L1I. One Opus agent, no subagents.

## Start state
Worktree build3ds/wt/perf2, branch perf2 from nds1 (current). Copy assets/us and ver/us/build into it. PROF2 (prof cost, one walk debug word, census lines off, integer report) runs in parallel in pm_phase.c, nd_walk.c, pm_gpu.c: do fix 0 only after PROF2 is merged (the orchestrator tells you); merge nds1 at each step boundary.

## Steps (premise check first; old path deleted in the same commit)
1. FIX 1 SUBMIT (pm_gpu.c ~974-1016 nd_native_submit). Suspects: (a) AUDIT W3 resets the 1,848 B shadow at every program change (prog_use), so pm4/pm5 alternation re-sends all template registers; (b) each pm5 draw copies its 588 B NdDraw into d5 and pm5_upload_draw sets citro3d uniforms, so C3Di_UpdateContext (1,256 B) runs per pm5 draw; (c) the two KICK splits per frame (C3D_FrameSplit, gpu_flush_written, svcFlushProcessDataCache, GSP) charged to a draw. Check: count program switches, shadow resets, UpdateContext calls and splits per frame (a counter line, Azahar count is valid). Change: NdShadow split into a common part (TEV, units, depth, blend) and a uniform part per program (pm4, pm5); pm5 uniforms through nd_emit uni() against the pm5 shadow at u5_* (W3's fix text), C3Di_UpdateContext not needed, d5 deleted (pass loc_uvx). Kick: one split, flush only the ring span written since the last split. Extra gates: nd_host nd_emit tests, battle_start_f182_cold.
2. FIX 2 ENTITY RESYNC. ent_run (nd_ent.c:365-376) calls pm_gbi_run per segment; nd_walk_run sets sc_ok = sr_ok = 0 (nd_walk.c:126) and bumps gv_epoch, so the next combine/render mode op fully resyncs and often closes the batch; memset(vert_live, 64) and dlc_dirty per key (:376). Check: full syncs per frame inside ent, and ent_gdl lists per frame. Change: nd_walk_ops(list, n) runs a flattened list's state ops without the DIP2 reset; pm_gbi_run stays for game task lists; memset(vert_live) only when the key has a segment with vertex loads. Extra gate: spot_mac01_ndmcheck.
3. FIX 5 D-CACHE LAYOUT. NdDraw (588 B) -> 64 B hot header (tmpl, unit 0 words, idx_word, nidx, locs, flags) plus a pm5 block pointer; NdNat (900 B) keeps the palette out of the entry (shared pal0, zmark rows in per-frame scratch); NdShadow -> valid bitmask plus the registers nd_emit writes (about 0.5 KB after step 1); NdMEnt: key hash, rp, visible mask in the first 32 B, key words in a side array; gs and nb (pm_gpu_int.h) per-draw fields first, 32-aligned. Check: sizeof lines in nd_host and the ndmsz line before/after.
4. FIX 4 I-CACHE. Out of the hot span: nd_model_state, nd_tev_build, nd_comb_decide, pm_dlc_native_ranges, C3Di_UpdateContext (after step 1). Cold-split nd_raw_native (4,704 B; keep the pm4 one-unit close inline, rect/pm5/two-unit/grab out of line, target < 2 KB), the pm_gpu_gtrip overflow/palette-full paths, pm_gpu_grange's wide copy. Order 3ds/ld/3dsx.ld.in by rate: per-op group, per-draw group, per-object group; per-op and per-draw within the first 12 KB. Add a map-sum tool (span bytes from pm_3ds.map) and print the span in each commit message; target <= 16 KB. No Thumb (ARMv6 Thumb-1 has no VFP).
5. FIX 3 SPRITES (pm_gpu.c:3216-3295). Per sprite today: flush of the open batch, nd_tex_get with a CI palette hash plus slot_valid re-hash after a PlayerRasterCache read (pm_tex.c:1197-1204), pm_gbi_gv_at, NdMCtx memset, pcomp, a 4-vertex ring load and its own submit. Check: memo hits/misses and dirty-slot rehash count per frame (tot_diff/tot_same exist). Change: texture id cache per sprite record (raster, palette, tex epoch -> id and unit words); consecutive same-state sprites batched into one draw (quads in the ring with their own palette entry, gtrip palette per vertex, pal_n up to the palette size). Extra gate: spot_mac01_sprcheck (repair its until-line first: no source prints it now).
6. FIX 0 SPLIT REST (after PROF2 merges; its debug word owns the bit). Class-change timers in the walk (one tick per change of op class, 150-300 per sampled frame, in the NDT exclusive scheme, nd.h:368-374): tex, comb, geo, mtx, rect, raw, list, plus kick (kick_check, cmd_split_forced) out of sub. Same line: program switches, shadow resets, UpdateContext calls, splits, sprite memo hits/misses, full syncs in ent, ent_gdl lists. Check: class sum plus rest equals gbi within 5% on spot_mac01_det.
kgr_01 rest (13-15 ms): only after row DARK and fix 0 name the class.

## Done criteria
Every step: build, nd_host PASS, byte-equal to the previous step on spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015; tour10_det prints 10 windows with the cls line. Hot-span bytes and struct sizes in the commit messages. Last: the user pushes spot_mac01_prof plus a mac_04/mac_05 spot (prof=30); the next target is the largest class left. Azahar timing never drops a fix (user rule); the console decides.
Affected scenarios (no others): the 4 byte gates, tour10_det, spot_mac01_ndmcheck, spot_mac01_sprcheck.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=perf2 PM_ROW=perf2 on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=perf2 PM_LANE=perf2 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=perf2 PM_LANE=perf2 python 3ds/tools/bench.py run <bench> --row perf2 --lane perf2 --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget perf2

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "perf2 ready <hash>".

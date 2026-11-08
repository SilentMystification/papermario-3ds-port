# Row PERF1: native draw path cost on the O3DS (oracle plan 2026-11-08)

## Goal
Console O3DS XL, 5c84d16e7, mac_01 wide (build3ds/hw_live_log_131.txt lines 315-509): fps 19.9, clean wk 45.8 ms, gbi 53.5 ms, oh 16.9 ms. Target: clean wk and ft under 33 ms, fps near 30. The user wants fixes, not comparison runs. One Opus agent does all steps, no subagents.

## Start state
Worktree build3ds/wt/perf1, branch perf1 from nds1 (this commit). Copy assets/us and ver/us/build into it.

## Steps (in this order; each: premise check first, then change, gates, commit; old path deleted in the same commit)
1. PROF COST + VISIBILITY. Delete the core-1 probe (pm_phase.c c1_main/c1_start/c1_busy, the core1_busy field; keep the core2 probe line): it spins svcSleepThread(0)/svcGetSystemTick on core 1 (GSP/DSP core). Profile "prof" in bench_defs.json: prof=30. In sampled frames replace the per-op hooks (nd_walk.c:145-163) and the ndt timer pairs (nd_recs.c:115-173, pm_gpu.c:967-976) by one timer per producer class: G_PM_REC, G_PM_ENT/ent_gdl, G_PM_SPR, dlc_try, native submit, rest. Print pmu e0 e1 cyc pm on their own line (the 256 B slot cuts the D-cache field, pm_phase.c:436). Gates: build, nd_host, spot_mac01_det byte-equal.
2. DEBUG WORK IN CLEAN FRAMES. pm_debug_has("r13off") per R13 record (nd_recs.c:140, pm_gpu.c:2724); pm_debug_has("mdl0") per why-2/3 record (nd_model.c:287,290); `pm_frame_count() % 300u` per record (pm_gpu.c:2815) and mdg_miss % 400; unconditional cold calls t1log_draw, pm5tex1_log, rp_draw per draw (pm_gpu.c:953-954,995). Change: cached flags, frame compare instead of %, flag test at the call site. Check: grep 'pm_debug_has(' 3ds/src/nd_*.c 3ds/src/pm_gpu.c.
3. ENTITY KEY HASH PER FRAME. ent_run (nd_ent.c:355-360) calls ent_hash (nd_ent.c:106-124) per key per frame over all list words (nested G_DL) and the whole vertex span (up to 0x20000 B). Premise check (1 Azahar start): add bytes-hashed per frame to the entnat line on spot_mac01_det; go on if > 64 KB/frame, else skip this step. Change: pm_walk_drain (nd_walk.c:414) records a write generation and range (nuPiReadRom gives dest and size); full hash only when a write overlaps the key's list or vertex range; per frame hash the list words plus the first 64 B of the vertex span. Extra gates: c2haz_mac01 no new address, entnat comp unchanged.
4. MODEL RECORD HIT PATH (nd_recs.c:113-185, pm_gpu.c:2670-2687, 2917-2949; 92 records/frame, 67 draw nothing). Three commits: (a) per-record-address memo (rp -> entry, last record words, state epoch bumped by the key setters) skips key build/hash/probe; (b) store each group's box and ranges in the entry, per frame only dlc_box_out per group gives a visible mask, compared with the entry's (replaces the pm_dlc_native_ranges re-walk, nd_shape.c:395-457); (c) mask 0 returns after rec_matrix (no tex lookup, tmpl build, flush, pcomp); nd_tmpl_build only when nd_tmpl_gen() changed. Extra gate: spot_mac01_ndmcheck diff 0.
5. SPRITE STATE MEMO. pm_gpu_spr_build (pm_gpu.c:3216-3275) via nd_model_state_in (nd_model.c:281-346) rebuilds comb/plan/tmpl/tev per sprite per frame. 16-entry memo keyed on comb w0/w1, omode, prim, prim_lod, env, rgb_scale, geom, tex present, r_mix/memblend/replace, cconst, mixcol, dview; value tmpl id, tev_col, buf_col, cmul/cadd, zmix, flags. Extra gate: spot_mac01_sprcheck.
6. SUBMIT. nd_native_submit (pm_gpu.c:948-997): C3Di_UpdateContext only when the context flags at __C3D_Context+32 have dirty bits other than DrawUsed or uniforms are dirty (check by objdump of libcitro3d.a first); nd_emit_draw writes into the GPUCMD buffer directly (room check ND_EMIT_MAX) instead of out[] plus GPUCMD_AddRawCommands. Extra gate: battle_start_f182_cold.

## Done criteria
Every step: build, nd_host PASS, byte-equal to the previous step commit on spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det shots 0000-0015 (pause_spin_grab_det_w300 is not a byte gate). Report A5 sizes (nm -S); the hot span stays at or under 16 KB (3ds/ld/3dsx.ld.in). Azahar times show direction only. A failed gate: 2 tries per cause, then drop that fix and go to the next step. Last: the user does one console prof push (spot_mac01_prof, O3DS XL).
Affected scenarios (no others): the ones named above, plus c2haz_mac01, spot_mac01_ndmcheck, spot_mac01_sprcheck.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=perf1 PM_ROW=perf1 on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=perf1 PM_LANE=perf1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=perf1 PM_LANE=perf1 python 3ds/tools/bench.py run <bench> --row perf1 --lane perf1 --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget perf1

## At the end
Delete this row file and build3ds/wt/perf1/.wt_keep in the last commit. SendMessage main "perf1 ready <hash>" with the per-step results.

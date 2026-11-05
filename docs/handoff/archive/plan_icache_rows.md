# plan_icache_rows: shrink and pack the per-model hot code (O3DS, Toad Town still view)

Plan only (Opus). Read at HEAD 41f50ffea (CAP2). No source edits, no runs, no git changes.
Rules: CLAUDE.md 2, 5, 7. hw_guidelines.md 1, 2, 10. Memory: no culling, no change to what is drawn.
Every row here moves code, changes a compile flag, or moves state. No row changes a value that reaches the GPU.

## 0. Source numbers

Console (build 1592 / CAP2 walk log, build3ds/hw_live_log_170.txt, still windows f12843 to f15543, phase on). MEASURED:

| Quantity | Value |
|---|---|
| sp0c tex / flush / dlc (replay) / rest, ms per frame | 7.8 / 0.9 / 4.9 / 3.2 |
| pmu window B (ev 16 1): stall-by-lack-of-instruction share = pmu_dc / pmu_cyc | about 0.41 |
| pmu window A (ev 0 11): I-cache misses per cycle = pmu_ic / pmu_cyc | about 0.0104 |
| Earlier still windows (plan_dl_rows section 0): B share range over 37 windows | 0.43 to 0.44 |

Cycle counts look high for 268 MHz. This plan uses the share only, not ms from cycles.

Code sizes: build3ds/pm_3ds.map (same build as build3ds/pm_3ds.elf, 2026-10-30 20:02). Bytes per source line: objdump -d -l of
build3ds/pm_3ds.elf (docker devkitpro/devkitarm, /opt/devkitpro/devkitARM/bin/arm-none-eabi-objdump), 4 bytes per ARM instruction,
literal pools included. Every byte count below marked M comes from this. The helper scripts are mapsz.py, agg.py and rng.py
(session scratchpad; agent_cfi.md "Commands" has the same method).

## 1. What a plain static model runs every frame

Build side (game thread, fsplit dl): execute_render_tasks (1284 B) calls appendGfx_model (5284 B) once per model. That loop with
its helpers is under 16 KB. It is not the I-fetch problem. Row MCEN only.

Replay side (gbi walk), per plain model, in order. "Span" = inside the CF4 hot span 0x100000 to 0x10db4c (3ds/ld/3dsx.ld.in 32-38).

| Step | Function (file) | Bytes (map, M) | Span | Runs |
|---|---|---|---|---|
| G_PM_MODEL dispatch | gbi_run_body (pm_gbi.c 3604) | 6860 | yes | per walker op |
| model words | mg_run (pm_gbi.c 3546) | 896 | no (0x13fbd4) | per model |
| same-value test | g2_same (pm_gbi.c 2833) | 972 | no | per word |
| lights | gbi_movemem 276, gbi_moveword 364 | 640 | no | per model with lights |
| G_PM_TEX | tb_run (pm_gbi.c 2794) | 480 | yes | per model (TBREP skips repeats) |
| texture words | tex_op (pm_gbi.c 2942) | 7848 | yes | per word of the list (8 to 12 words) |
| combine | sync_combine 1288, sync_combine_work 1352, sc_sig_now 364 | 3004 | yes | per TEXTURE / SETCOMBINE word |
| combine change | decide 2948, pm_gpu_set_plan 1612 | 4560 | no | when the combine signature changes |
| G_PM_RM | rm_apply (pm_gbi.c 2872), pm_gpu_set_mode 128 | 684 | no | per model |
| G_MTX / G_POPMTX | load_mtx 472, pm_gpu_set_mvp 520, gbi_popmtx 176 | 1168 | no | per model |
| G_DL replay | dlc_try 1956 (yes), dlc_run_group 940, dlc_static_group 632, dlc_box_out 504, dlc_seg 700 (no) | 4732 | part | per group (125 per frame) |
| draw epoch | gv_epoch_enter 400, gv_state_why 584, gv_params_calc 1816, gv_matrix_id 880 | 3680 | yes | per epoch |
| texture bind | ensure_tile 1168, tile_bound 388, dual_bind 1704, sync_render 1068, pm_tex_load 92, load_impl 4084 | 8504 | yes | per new bind |
| index copy | pm_gpu_grange 1772, draw_tri 740 | 2512 | yes | per group / per tri |
| close | pm_gpu_flush_at 476, flush_impl 268 (no), flush_draw 13712, pm_tex_bind 1352 | 15808 | part | per close (77 per frame) |
| close helpers | kick_check 488, sh_* 704, pm_tex_i8 232 (no); C3Di_UpdateContext 1256, C3D_DrawElements 540 (yes); C3D_UpdateUniforms 656 (no, 0x3d98ac) | 3876 | part | per close |

Sum of these function bodies: 61.3 KB (M). Of this, 9.1 KB of per-model callees are outside the CF4 span (M). The L1I is 16 KB,
4-way, 32-byte lines. A body larger than the lines it runs still costs: cold blocks sit between hot blocks, so hot lines are
partly empty and the hot set spreads over more sets of the cache.

Inline debug and phase code inside these bodies (M, objdump lines that carry pm_ph_on, PH_*, RS_ADD, GP_SET, DC_T, gp_on, dd_on, dls_on):
gbi_run_body 1924, flush_draw 1136 (pure instrumentation lines; 3988 with mixed lines), tex_op 744, gv_tri 676, ensure_tile 620,
sync_combine 320, mg_run 196, dlc_try 188. The lazy debug-flag tests alone (gp_on, dd_on, xt_on, po3a_on, rmlog_on, fl_on, nozt,
zmix_y_dbg inlined with their pm_debug_has / sscanf init) are 1312 B over 7 hot functions (M).

Phase on adds the hook bodies to every walker op: os_op_hook 396, mops_count 1044, mdc_op 844, gv_streak_op 128, tex_streak_op 184
(2.6 KB, M). The B share 0.41 was read with phase on. So part of the stall is the measurement itself.

## 2. Step 0 (no code): phase-off baseline

One console push of the current build with `pmu` and without `phase` (same still view, 4 windows minimum: 2 A, 2 B). It gives the
phase-off B share. The epic decision uses the same push arguments before and after. If the phase-off B share is under 0.25, the
epic ceiling is small: run rows 1 to 3 only. (plan_dl_rows section D gives the same request.)

## 3. Rows (ordered by bytes removed per risk)

Risk: low = moved code or a compile flag, the same C semantics, byte-equal pictures expected. Medium = a value is read at another time.
Picture gate for every row: azt HEAD run twice, the two HEAD shots byte-equal first (shot_cmp.py, top shots). Then the row build,
byte-equal against HEAD. Scenes: mac01t_cap1 and yard for every row. pm_gpu.c rows add pause_menu (2D rects use the CPU close path).
pm_tex.c and model.c rows add forest_mim. Azahar has no cache model: its timing never decides a row here.
Map gate before any Azahar start: the map quantity of the row must hold, else the row stops with no start used.
Console decision: one push at the epic end (rule 6), plus `pmu`. The decisive number is the B share (section 4).

| # | Row | Files and lines | Bytes off the hot path | HW reason | Risk | Map gate | Console quantity |
|---|---|---|---|---|---|---|---|
| 1 | OS1 hot files at -Os | 3ds/CMakeLists.txt 9 (add one set_source_files_properties for 3ds/src/pm_gbi.c, pm_gpu.c, pm_tex.c: COMPILE_OPTIONS -Os). 1 to 3 lines | E: 9 to 15 KB of the 61.3 KB (15 to 25 % smaller ARM code; not built in this plan) | I-cache fit: fewer lines per model. hw_guidelines 1 names -Os for cold files; this row extends it to the hot files, and the B share decides | low for pictures (no -ffast-math; VFPv2 vmla is not fused, so float results are equal). Speed risk: no loop unroll, small memcpy as calls | sum of the section 1 bodies down 8 KB or more (mapsz.py) | B share down 0.02 or more; sp0c tex, dlc, rest each not up more than 2 % |
| 2 | FLH flush_draw split | pm_gpu.c 751-1049: to noinline flush_draw_cpu(): CPU-vertex tail 993-1049 (1592). To cold noinline: off_active snapshot 774-780 (488), light-table upload 799-807 (176 + uniforms.h inlines), pcomp recompute 815-822 and 842-849 (388), DC_T(1)/DC_T(2) sample bodies 872, 886 as `if (dc_on) dc_mark(k)` (556), fl_on block 754 (88), nozt 889 (44), off_active scissor 926-933 (132) and blend 950-954 (108) | M: 3572 of 13712 (26 %) | hot code contiguous: the raw-batch close path becomes one block; cold blocks out of line | low: moved code. Shared draw code: byte-equal on mac01t_cap1, yard, pause_menu | flush_draw 10.2 KB or less | sp0c flush down 5 % or more (0.9 ms base) and tex not up; B share down |
| 3 | TEXH tex_op split | pm_gbi.c 2942-3333: to noinline tex_tile1() the tile 1 block 3062-3125 (1056); to cold noinline: shaded sprite 3129-3147 (312), S25 negative window 3041-3052 (156), V20 palette loop 3022-3031 (200); delete the dead imglog block 3058-3061 (56, `on` is never read); GP_SET bodies 2962, 3053, 3219, 3222, 3271, 3278 to one cold gp_note(k, same) (444) | M: 2224 of 7848 (28 %) | I-cache fit: a plain bind runs SETTIMG, SETTILE, LOADTLUT, LOADBLOCK, SETTILESIZE tile 0, TEXTURE, SETCOMBINE, OTHERMODE_H. These lines become dense | low: moved code, one dead block deleted | tex_op 5.7 KB or less | sp0c tex minus flush down 3 % or more; B share down |
| 4 | DBGF debug flags as plain globals | pm_gbi.c 18-20 (gp_on), 90-112 (xt_on, dd_on), 327 (po3a_on), 1262 (rmlog_on), 25; pm_gpu.c 101 (zmix_y_dbg), 413 (fl_on), 889 (nozt). One cold pm_dbg_frame() at frame start (pm_gbi_end_frame entry) writes one PM_HOTD flag word; the inline tests read it. dd_on / xt_on keep the frame window test, done once per frame | M: 1312 (lazy init bodies in tex_op 200, flush_draw 344, gbi_run_body 436, sync_combine 208, sync_combine_work 48, draw_tri 44, mg_run 32) | I-cache fit; D-cache: the flags in one hot line | low to medium: drawlog / xtrace windows are decided per frame, not per op (debug output only, no picture change) | the 7 bodies down 1.2 KB or more | phase-off B share down (phase-on fields cannot show it) |
| 5 | HOOK1 walker hooks in one cold call | pm_gbi.c gbi_run_body 3607-3823: the dls_on lines 3607, 3609, 3647, 3678, 3714, 3728, 3747 (320), phase lines 3643, 3654, 3662-3664, 3668, 3670, 3715, 3718, 3732, 3823 (about 650) into `if (pm_ph_on \| dls_on) walk_hook(...)`; gv_tri 1468-1538 GP_LAP and PH_ADD (676) into one cold call per exit | E: 1.2 to 1.6 KB (instrumented lines M: gbi_run_body 1924, gv_tri 676) | hot code contiguous: the op loop body gets shorter | low: the same counters, moved | gbi_run_body 5.6 KB or less, gv_tri 2.0 KB or less | phase-off B share down; with phase on, no change expected |
| 6 | LAY2 hot list complete and in call order | 3ds/ld/3dsx.ld.in 32-38: add mg_run, g2_same, gbi_movemem, gbi_moveword, rm_apply, pm_gpu_set_mode, load_mtx, pm_gpu_set_mvp, gbi_popmtx, dlc_run_group, dlc_static_group, dlc_box_out.*, dlc_seg, kick_check, flush_impl, sh_*, pm_tex_i8, C3D_UpdateUniforms, then (after FLH and TEXH) flush_draw_cpu and tex_tile1 last. Order the list as section 1. No C change | M: 0 removed; 9120 B of per-model callees moved into the span (M). Span 57.7 KB to about 66.8 KB | TLB and set spread: the per-model path in one address range, C3D_UpdateUniforms no longer 3.7 MB away | lowest | span start 0x100000, the 18 names inside it | B share down; window B pmu_ic (main TLB miss, 926 to 958 per frame) down |
| 7 | TST1 tile and combine state in one block | pm_gbi.c 182, 185, 267-272, 288-289, 311, 366: tile_fmt, tile_siz, tile_tmem, tile_pal, tile_sz, tile_wrap, tile_shift, ldrec, blk, comb_a..comb_d, comb2_*, timg_* into one struct, 32-byte aligned, in .data.pmhot (CF5 block). Types unchanged | 0 code; about 560 B of state in about 18 consecutive lines (E) | D-cache: state kept in one block, read in order by tex_op | low to medium: many names change (a missed rename does not compile) | none (data) | window A pmu_dc (D-cache read miss, 54 to 59 k per frame) down 5 % or more |
| 8 | MCEN appendGfx_model census out of line | src/model.c 1413-1431 (PT40, MDL1) and 1498-1510 (MR0, MS0) into one cold noinline mdl_census() in a TARGET_3DS block, called under `if (pm_ph_on \|\| modellog)` | M: 856 - call (about 840) of 5284 | hot code contiguous on the build side | low: TARGET_3DS block only, the decomp code is not changed | appendGfx_model 4.5 KB or less | fsplit dl ms (4.3 base); low value |

Bytes per risk, in order: OS1 (E 9 to 15 KB, low), FLH (M 3.6 KB, low), TEXH (M 2.2 KB, low), HOOK1 (E 1.4 KB, low), DBGF (M 1.3 KB,
low to medium), LAY2 (0 removed, 9.1 KB packed, lowest), TST1 (data), MCEN (M 0.8 KB, build side).

Notes per row:
- OS1 first because one flag line moves the most bytes. After OS1 the byte counts of rows 2 to 5 change. The row agent measures them
  again with mapsz.py and rng.py before it moves code. If OS1 fails the console gate, revert it and run rows 2 to 6 on -O2.
- FLH: the CPU-vertex tail is noinline, not cold (sprites and rects use it each frame). The raw-batch path returns at 991, so the
  plain model close never reaches 993-1049. The sampled close (DC_T) keeps its tick marks; only the body moves.
- TEXH: tex_tile1 is noinline, not cold: aux and mipmap textures (RENDER_CLASS_2CYC, counted as plain in mr0) use tile 1. The
  SETCOMBINE decode (3298-3328, 988 B) stays inline: every bind list that TBREP does not skip runs it.
- DBGF: gp_on, po3a_on and rmlog_on read the switch once now and never again; the global keeps that. PMDL("dlsrc") follows
  pm_dbg_gen; pm_dbg_frame() reads it once per frame with the same test.
- HOOK1 and DBGF help only when phase is off. The user judges speed with phase off, so the phase-off B share decides them.
- Not proposed: Thumb code (ARMv6K Thumb-1 has no VFP access and few registers); moving decide / pm_gpu_set_plan into the span
  (they run only on a combine change); any skip of a model, group, list or triangle (memory rule).

## 4. Epic and the number that decides it

One code agent, one epic ICF2: OS1, FLH, TEXH, HOOK1, DBGF, LAY2 (TST1 and MCEN only if the epic passes and the user wants more).
Each row: map gate, then the pictures (HEAD-against-HEAD, then row against HEAD). No Azahar timing gate.

Console push at the epic end: the rule 6 command plus `pmu`, same still view. Step 0 gives the phase-off baseline with the same arguments.

The number that decides the epic: pmu window B share = pmu_dc / pmu_cyc (ev 16 1, STALL_BY_LACK_OF_INST over cycles) in the
still view.
- Pass: the share drops 0.03 or more against the same-argument baseline (0.41 phase on; the step 0 value phase off), in 2 of 2
  B windows. The window-to-window spread is under 0.01 (0.4270 and 0.4270 in inv_pmu_events; 0.43 to 0.44 over 37 windows).
- Support: window A pmu_ic / pmu_cyc down (0.0104 base), sp0c tex (7.8), dlc (4.9), rest (3.2) not up, fsplit gbi down.
- Fail: the share drops less than 0.01. Then revert the rows that the console cannot see (OS1 first), and keep the moved-code rows
  only if the user wants the smaller binary.

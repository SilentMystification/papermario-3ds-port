# plan_dl_rows: small rows for display-list build and replay (O3DS, Toad Town)

Plan only (Opus). Read at HEAD e5802e650 (code of build 1592 = 20703d076 SPLIT0C2). No source edits, no runs.
Rules: CLAUDE.md 2, 5, 7. hw_guidelines.md. Memory: no culling, no group skip. No row here adds or removes a skip of a model,
group, list or triangle. Every word the game emits keeps its effect.

## 0. Source numbers (console log build3ds/hw_live_log_170.txt, build 1592, git 31bad3c62)

Still windows: f24492 to f35292 (log lines 11492 to 17360), `pos M x 32 y 0 z 85 | C x 32 y 103 z 472`, 37 windows of 300 frames.
All windows have `phase` on. Values are per frame. Range = min to max over the still windows.

| Line | Field | Value |
|---|---|---|
| fsplit | gbi / dl / logic / wall ms | 46.9-48.2 / 4.3-4.4 / 3.2 / 56.2-57.5 |
| opsplit | ops; class ms/ops | 1322-1349; tex 7.4-8.1/180-189, mtx 2.2-2.4/115-117, tri 3.1-3.4/101, cc 1.6-1.9/57-63, oth 27.6-28.7/183 |
| opsplit | top op ms | 0c (G_PM_MODEL) 22.3-23.3, 0b (G_PM_TEXI) 6.6-7.1, 06 3.1-3.4, 01 2.7-2.9, e4 2.4-3.0, 09 (G_PM_RM) 2.0-2.3 |
| opsplit | pt (svc tick reads) | 1710-1732 |
| sp0c | tex / dlc / rest / flush us | 10537-10598 / 7281-7450 / 3273-3302 / 1133-1157 |
| sp0c | skip / grp per window | 14910 / 37500 (49.7 and 125 per frame) |
| mr0 | plain models; us per model ops / close | 90; 196-199 / 55-57 (90 x 254 = 22.9 ms) |
| ms0 | opaque plain / runs / removable closes / texture changes | 70-72 / 13 / 15 / 47-48 |
| mops | cc / cc_same / mtx / mtx_same / bind / tex_same | 14-15 / 2 / 115-117 / 1 / 0 / 0-2 (top-level ops only) |
| mdlsum | models / static | 105-106 / 102-103 |
| dcsum | closes; us per close pal+bind+tev+st+draw | 73-77; 15+9+8+12+35 = 79 (1 close in 8 sampled, 8 tick reads per sample) |
| dcsum | cmd words per close; GSP flushes | 70-71; 7 at 19-23 us |
| dcgrp | per close: shadow miss, bind0, tev, palup, palsame, pmul | 1.30, 1.10-1.11, 0.46-0.47, 0.48-0.50, 0, 0.28-0.29 |
| dlc2 | per window: tri, opsrep, static grp, batch | 259880, 340800, 22590, 6600 (866, 1136, 75.3, 22 per frame) |
| phase count | tri / draw / texcall / gvload / gvtri / gvrej / gvfb | 1058-1082 / 106-108 / 86-89 / 327 / 880-904 / 162 / 16 |
| pmu A | I-cache miss / D-cache read miss / cycles | 151505-164820 / 57183-59536 / 14.36M-14.95M |
| pmu B | main TLB miss / stall for instructions / cycles | 926-958 / 5.79M-6.55M / 13.44M-14.92M |
| texstat, tx1 | hits per window, misses | 24253-26047, 0 |

Derived (no new data):
- Instruction stall is 6.53M of 14.86M cycles (44 %) = 24.4 ms of a 55.8 ms frame at 268 MHz (pmu f25200). Stall per I-miss:
  6.53M / 164.6k = 39.7 cycles = 0.148 us.
- Close cost in the frame: 77 x 79 us = 6.1 ms. Close cost inside plain models: mr0 90 x 56 us = 5.0 ms.
- Tex handler time inside groups: sp0c tex - flush = 10.56 - 1.15 = 9.4 ms. Plus 0b 6.85 ms outside groups.
- Phase cost: this log has no phase-off window. Build 1587 (other log, plan file) had gbi 33.8 to 34.6 ms phase off. Here gbi is
  47.1 ms phase on. Every ms value below is phase on. The phase-off value is lower by an amount this log cannot give.
- Code size of one plain model's path (build3ds/pm_3ds.map, same source): tex_op 7828 B, flush_draw 13712 B, gbi_run_body 6860 B,
  gv_params_calc 1816, pm_gpu_grange 1772, dlc_try 1956, sync_combine 1288, pm_gpu_set_plan 1612, pm_tex_bind 1352,
  C3Di_UpdateContext 1256, ensure_tile 1148, sync_render 1068, g2_same 972, mg_run 896, gv_matrix_id 880 and 13 smaller.
  Sum about 43.7 KB against a 16 KB L1I. Not in the CF4 hot set (3ds/ld/3dsx.ld.in 32-38): mg_run (0x13f860), g2_same
  (0x13a098), dlc_seg, dlc_static_group, dlc_box_out, decide, rm_apply, load_mtx, gbi_moveword, gbi_movemem, gbi_popmtx,
  pm_gpu_set_plan, flush_impl, C3D_UpdateUniforms (0x3d9354).

Main finding: the walk is instruction-fetch bound (44 % of cycles). Each of the 90 plain models runs about 44 KB of handler code
once, so the cache refills per model. Arithmetic is small. A row saves time when it stops a handler from running (dedup) or makes
the code that runs denser (layout). Work that the PICA200 could take (class iii) is small by count (section A3).

## A. Census method (design only, no run yet)

One cold, phase-only line `dup f<n>` per 300 frames, printed beside sp0c (pm_gbi.c 2374, after sp0_report). Counters are plain
increments. No tick reads. Each "same" test runs only when `pm_ph_on`, in a cold noinline function, and never changes state.
Row DUP0 adds it (about 60 lines, pm_gbi.c and one TARGET_3DS block in src/model.c).

### A1. Values re-derived or re-checked each frame with no change

| Class | Check (file:line) | Census counter (new) | What the log counts now |
|---|---|---|---|
| Render class, plain test | src/model.c 1504-1505 (per model) | none needed | mr0 plain 90, mdlsum models 105: 105 tests per frame |
| Texture state, whole list | tb_run pm_gbi.c 2778 (from mg_run 3550, walker 3668) | `tl` lists, `tl_rep` = same w1 as the last list and no texture writer since (writer gen bumped at tex_op 2918 entry, rm_apply 2848, gbi_run_body 3594, dual_bind 1155, ensure_tile 2624, pm_tex epoch) | ms0: 48 texture changes in 71 opaque models in 13 runs, so 10 adjacent same-texture pairs. Nested lists not counted (mops bind 0) |
| Texture state, per word | tex_op 2918: SETTILE same_tile 2934, SETTILESIZE tsz_last 3188, glyph 3178 | `tx` tex_op calls, `tx_g2` calls that g2_same 2809 would skip | not counted (opsplit tex 180 ops counts top level and mg_run words, not tb_run words) |
| Combine index | sync_combine 1106 (sc_sig 1110, scm memo 1115), SETCOMBINE decode 3252-3305, g2_same 2813 | `sc` calls, `sc_hit`, `scm_hit` | mops cc 14, cc_same 2 (top level only). gbiprof GP_SET(5) is off in this push |
| Tint | model.c 1476-1496 words, g2_same 2828-2830 | `g2` words skipped by g2_same (walker 3613 and mg_run 3526) | not printed (RS_SKS exists, no log line) |
| Fog | model.c 1810-1811, mg_run 3547 (fogc), gbi_moveword 3505 (G_MW_FOG has no g2 case) | inside `g2` | mdlsum fog 0 in the top 12 tasks: no fog in this view |
| Lights | gbi_moveword 3503 (NUMLIGHT, no g2 case), gbi_movemem 3480, g2_same 2822 | `nl` NUMLIGHT calls, `nl_same` | mops lt 0 (top level only) |
| Render mode list | rm_apply 2848 (tex_op at 2864 without g2_same) | `rm` calls, `rm_same` (omode_l, omode_h, geom, comb_w0, comb_w1 equal after the apply) | opsplit 09: 2.0-2.3 ms top level, count not printed |
| Matrix | load_mtx 1881 (unpack 421, mul), upload_mvp 426, gv_matrix_id 1391 | `mx` calls, `mx_same` (the 64 raw bytes equal the last bytes at the same pointer, 16-entry table by pointer) | mops mtx 115 (top level), mdlsum static 103 of 105, opsplit gms 19-22 gmn 58-61 |
| Draw epoch | gv_epoch bump mg_run 3530, walker 3633; gv_epoch_enter 1442 (gv_state_why, tri_flags, ensure_tile, dual_bind, sync_render); gv_params_calc 1262 (from dlc_seg pm_dlc.inc.c 218-224 and gv_tri 1509) | `ep` enters, `ep_same` (ep_why, ep_ct, ep_cullboth, ep_fl and the bound texture slot equal to the last enter); `gp`, `gp_same` (GvP memcmp equal) | not counted |
| g2_same prologue | walker 3613, mg_run 3526 | `g1`, `g2` skipped words | opsplit ops 1323 = words that passed both skips |
| GP writes | flush_draw pm_gpu.c 751: sh_* shadow, TEV key 899, pal key 823/850, pm_tex_bind tb_last | none new | dcsum cmd 70 x 77 closes = 5390 words; dcgrp shadow miss 100, TEV rebuild 35, palup 37, palsame 0, bind0 85 per frame. These are deduplicated now |
| Render task sort | src/model.c 4823-4880 (three exchange sorts, O(n^2), every frame) | `rts` tasks, `rts_same` (the dist sequence equals last frame's) | not counted. n is 105 or more (mdlsum models) |
| Group box test | pm_dlc.inc.c 473 dlc_box_out | none | sp0c grp 125, skip 49.7. Census only: existing skip, not a row |

### A2. Words emitted and then not used

| Class | Where | Count now |
|---|---|---|
| PIPESYNC per model | src/model.c 1518, 2065; skipped by g1_skip 3526 | 2 x 105 models = 210 words per frame (mdlsum) |
| Duplicate SETCOMBINE | src/entity/Shadow.c 61-63 (3 equal words in the circle shadow run); all run in tb_inline 2789 | not counted (`ti`, `ti_g2` in DUP0) |
| Debug calls with the switch off | imglog 2924 (cold call per SETTIMG), the unused `imglog` block 3033-3036, suntile_t0 3165 (per tile 0 SETTILESIZE), suntile_tri 1482 (per gv_tri) | not counted |
| Static index copy | pm_gpu_grange pm_gpu.c 1990: memcpy of the static group indices into gidx | dlc2 tri 866 per frame = 2598 indices = 5.2 KB per frame |
| Groups out of the box | dlc_box_out | 49.7 per frame. Existing skip, out of scope |

### A3. CPU work the PICA200 does anyway

| Work | Where | Count now | Note |
|---|---|---|---|
| Clip reject per vertex and triangle | vert_cb at G_VTX, gv_tri 1461, dlc_box_out | gvrej 162, skip 49.7 | This is culling. Out of scope (memory rule) |
| mv x proj per G_MTX | upload_mvp 426 | mtx 115 top level + about 180 nested | 64 multiply-adds each. Small |
| post x game per palette entry | pm_gpu.c 819, 846 | dcgrp pmul 0.28 x 77 = 21.6 per frame | about 0.02 ms. Not a row |
| Light table to float, 27 divides by 127 | gv_params_calc 1297-1306 | per lit epoch, not counted | Small. Folded into EPO1 |
| Index and vertex copy to the ring | pm_gpu_grange, pm_gpu_gvtx_load | 5.2 KB + 327 x 16 B per frame | Small |

Result of A3: no row. The PICA already does transform, lighting, texgen, fog and combine (TEV). The second core is not used:
O3DS core 1 is the system core with a small slice (hw_guidelines). Every candidate reads or writes the gbi state that the
GPU-call path uses. The only separable work (task sort) is cheaper to memoize (SORT1) than to move.

## B. Rows (ranked)

Saving = console ms, phase on, from the still windows. M = MEASURED (product of log values), E = ESTIMATED (stated assumption), U = UNKNOWN.

| # | Row | What repeats | File:line | Saving (source) | Reason | Gate quantity | Scenario | Risk |
|---|---|---|---|---|---|---|---|---|
| 1 | BAT1 sort opaque run by texture | a close (GPU state emit, 70 words) for a texture that comes back later in the same opaque run | src/model.c 4882 (after the sorts in execute_render_tasks): stable sort of each run of plain opaque appendGfx_model tasks by textureID + textureVariation. About 30 lines | M: up to 1.19 ms = ms0 removable closes 15 x dcsum 79 us. Upper bound (ms0 comment) | GPU defer: fewer closes, same draws | dcsum `cl` per frame down 10 or more | mac01t_dcsum; byte-equal mac01t_dcsum, yard, forest_mim | medium: order of opaque models changes; equal-depth faces can change pixels (byte-equal catches it) |
| 2 | G2T g2_same in list runners | a texture, combine or othermode word that sets the value already held, run through tex_op and sync_combine | pm_gbi.c 2782 (tb_run), 2792 (tb_inline), 2864 (rm_apply): `if (g2_same(op, w0, w1)) continue;` before tex_op. About 4 lines | E: 0.5 to 2.0 ms = 3 to 12 % of tex time 16.3 ms (sp0c tex - flush 9.4 + 0b 6.85). Assumption: 30 to 60 % of these words are equal (3 of 9 in a model list, 10 of 17 in a shadow run) and they are the shorter handlers | dedup; fewer tex_op entries keep tex_op lines in L1I | DUP0 `tx` tex_op calls per frame down 25 % or more | mac01t_cap1; byte-equal mac01t_cap1, yard | low: the same test the walker uses (G2, R2B) |
| 3 | TBREP same list again | a full G_PM_TEX list (8 to 10 words, SETTILESIZE tail, tsz_last test) that rebinds the texture already bound, with no texture write between | pm_gbi.c 2778 tb_run; skip test at 3550 and 3668; writer gen at tex_op 2918, rm_apply 2848, gbi_run_body 3594, dual_bind 1155, ensure_tile 2624 and pm_tex_epoch(). About 25 lines | E: 0.3 to 1.0 ms. After BAT1, adjacent same-texture pairs = 71 - 13 - (48 - 15) = 25 per frame (ms0). Assumption: a repeat list costs 15 to 40 us after G2T (mean bind 9.4 ms / 90 = 104 us, a repeat has no flush and no load) | dedup | DUP0 `tl_rep` lists skipped per frame 20 or more; check switch `tbcheck` (run the list too, compare tile, td, comb, omode fields) bad 0 | mac01t_cap1; byte-equal mac01t_cap1, yard, forest_mim | medium: one missed texture writer gives a wrong texture |
| 4 | EPO1 epoch bump only on change | gv_epoch_enter + gv_params_calc (about 5 KB of code) per model when the model's words changed nothing | pm_gbi.c 3530 (bump before every word), 3549-3554 (G_PM_TEX, G_PM_RM): no bump when TBREP skipped the list or rm_apply changed no field. About 15 lines | E: 0.2 to 0.6 ms. Assumption: 25 to 60 enters per frame avoided at 5 to 10 us each | dedup; cache | DUP0 `ep` enters per frame down 20 % or more; `gv1check` stale 0 | mac01t_cap1; byte-equal mac01t_cap1, yard | medium: a stale draw decision; gv1check exists for it |
| 5 | SORT1 memo of the task sort | three O(n^2) exchange sorts of n >= 105 tasks each frame with the same keys | src/model.c 4823-4880: keep last frame's dist sequence and sorted[]; equal sequence = reuse sorted[] (same output by construction). About 20 lines | E: 0.1 to 0.5 ms, phase off too (game code). Assumption: n 105 to 150, 10 to 15 cycles per compare, passes 2 and 3 add up to 1x | dedup (build side) | DUP0 `rts_same` 300 of 300 frames in the still view | mac01t_cap1; byte-equal mac01t_cap1, yard | low: output is the same permutation for the same keys |
| 6 | TEXH tex_op hot and cold split | 7828 B handler refilled for each bind | pm_gbi.c 2918-3307: move to cold noinline (code unchanged) SETTIMG frame-buffer branch 2923-2926, LOADTLUT V20 loop 2997-3006, S25 negative window 3016-3027, tile 1 block 3037-3100, shaded sprite block 3104-3121, glyph branch 3177-3187; SETCOMBINE decode 3253-3305 to comb_decode(); delete the unused imglog block 3033-3036; guard suntile_t0 3165 with an inline flag | E: 0.3 to 0.8 ms = 25 to 50 I-misses per bind x 100 binds x 0.148 us (pmu) | cache (L1I, hw_guidelines 1, 10) | console pmu A I-cache miss per frame down 5 % or more | mac01t_cap1 (picture, byte-equal); console push `pmu fsplit` | low: moved code |
| 7 | FLH flush_draw hot and cold split | 13712 B close function (86 % of L1I) refilled per close, evicting the walk code | pm_gpu.c 751-1030: move off_active snapshot 775-780, decal2 1004-1016, zmark, dview 708-721, off_active scissor 926-932, the CPU-vertex path 993-1003 to cold noinline. About 40 moved lines | E: 0.3 to 1.2 ms = 77 closes x 25 to 100 I-misses x 0.148 us | cache | console dcsum us per close (st + pal + draw) down 10 % or more | mac01t_dcsum; byte-equal mac01t_dcsum, yard, forest_mim | low: moved code; shared draw code, so byte-equal on 3 scenes |
| 8 | LAY1 hot set update | none (layout); the per-model path is split over 0x100000 to 0x3d9354 | 3ds/ld/3dsx.ld.in 32-38: add mg_run, g2_same, rm_apply, gbi_moveword, gbi_movemem, load_mtx, gbi_popmtx, dlc_find, dlc_seg, dlc_static_group, dlc_box_out.*, pm_gpu_set_plan, pm_gpu_set_mode, flush_impl, C3D_UpdateUniforms in call order. No C change | U: 0 to 1.2 ms (0 to 5 % of the 24.4 ms instruction stall) | cache, TLB | console pmu B stall for instructions per frame down | mac01t_cap1 byte-equal; console push `pmu fsplit` | lowest |
| 9 | MTXM matrix unpack memo | unpack and mul of a static model matrix that did not change (finalMtx = &model->savedMtx, model.c 2825) | pm_gbi.c 1881-1906: 16-entry table by pointer, 64-byte compare, reuse the float matrix | E: 0.1 to 0.3 ms. Assumption: 180 G_MTX per frame, 103 of 105 static (mdlsum), 300 to 500 cycles saved each | dedup | DUP0 `mx_same` 90 or more per frame | mac01t_cap1; byte-equal mac01t_cap1, yard | low |
| 10 | GEN0 words nobody uses | 2 PIPESYNC per model; 2 extra equal SETCOMBINE in the circle shadow | src/model.c 1518, 2065 (TARGET_3DS: do not emit); src/entity/Shadow.c 62-63 (TARGET_3DS block: one SETCOMBINE, n 18 to 16) | E: under 0.05 ms. 210 words per frame not written and not read | dedup (build side) | DUP0 `g1` skipped words per frame down 200 or more | mac01t_cap1; byte-equal mac01t_cap1, yard | low |

Row DUP0 (census, diagnostic, not ranked): section A. Gate: the `dup` line prints in mac01t_cap1 with `tx`, `tl`, `sc`, `ep`,
`mx`, `rts`, `g1`, `g2` above 0. One Azahar start (budget 3). It rides in the BAT1 epic console push.

Rows not made (and why): a box-test memo (dlc_box_out is about 60 multiply-adds x 125 groups: under 0.05 ms, and it touches the
existing skip); post x game on the GPU (0.02 ms); TexBind dedup (pm_tex_bind already keeps tb_last, palsame 0); citro3d draw cost
35 us per close (library code, not in the repo).

## C. Order

Epic A (dedup and closes, one code agent): 1 BAT1, 2 DUP0, 3 G2T, 4 TBREP, 5 EPO1.
- BAT1 first: the only MEASURED saving (1.19 ms upper bound), 30 lines, a count gate that Azahar shows exactly (dcsum cl).
- DUP0 next: it gives the counts that turn the E values of G2T, TBREP, EPO1, SORT1 and MTXM into measured counts. A row whose
  DUP0 count is under its gate does not start.
- TBREP after BAT1 and G2T: BAT1 makes 15 more same-texture pairs adjacent, G2T removes the cheap part first.
- Console push at the epic end: `PM_ROW=PUSH sh .../push_3ds.sh --no-build --ip 192.168.1.170 phase fsplit pmu loglevel=debug`,
  same still view. Quantity: fsplit gbi and dcsum cl against section 0.

Epic B (build side): 6 SORT1, 7 MTXM, 8 GEN0. Console quantity: fsplit dl (SORT1) and gbi.

Epic C (cache layout, console-only gate): 9 LAY1, 10 TEXH, 11 FLH. Azahar has no cache model: the Azahar gate is byte-equal
pictures only. The console push runs `pmu fsplit` without `phase` (the phase hooks are cold calls per op and would hide the
change). Start epic C only after the section D counter shows that most of the instruction stall is in the walk and the closes.

## D. What the log cannot tell

1. Phase-off times. Every window has phase on (1715 svc tick reads per frame, 5 cold hook calls per walker op). gbi is 47.1 ms
   here and 33.8 to 34.6 ms phase off on build 1587 (different build). All savings above are phase on.
2. Where the 6.53M stall cycles happen: walk, closes, game logic (dl 4.3, logic 3.2 ms) or the phase hooks. pmu is per frame.
3. Counts of nested words by class and their same-value share (mops and mdc_op see the walker only). DUP0 adds these.
4. The 0b time (6.85 ms) per producer: shadow, sprite, hud, glyph.
5. Closes inside plain models and empty closes (pm_fl_n, pm_ec_n are not printed). mr0 gives only the time (56 us per model).
6. The task count per frame and the sort time.

The one extra counter that matters: a PMU split by group. In sp0_group_begin and sp0_group_end (pm_gbi.c 2134-2135, phase only)
read PMN0 and PMN1 (svcControlPerformanceCounter GET_VALUE) and add the difference. Print `imiss` and `istall` (pmu window A
and B) on the sp0c line. It tells how much of the 24.4 ms instruction stall is inside the 125 model groups. If it is under 30 %,
epic C does not start. Cost: 2 svc reads per group (250 per frame), phase only. Also run one push with `pmu fsplit` and no `phase`
(no code) to get the phase-off pmu baseline for epic C.

## Decision (2026-10-31)
- TBREP KEPT. Passed picture gate (mac01t_cap1, yard). Measured sp0c tex down about 3 percent. Skip count 19.8 per frame against written gate 20: the shortfall is recorded, the change stays.
- EPO1, SORT1, MTXM, GEN0, DBGF, HOOK1, MCEN: dropped on their gates (see agent_dlm.md, agent_gen0.md, agent_cfp.md).
- LAY2 and TST1 KEPT: data moves only, no code change, pictures equal to HEAD, no measured gain.

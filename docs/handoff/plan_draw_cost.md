# Plan: cost of a non-empty batch close (draw cost), O3DS yard

Branch dlc-compiler-c1c2. Plan only. Tags: [M] measured (source named), [C] read from code (file:line), [L] citro3d library behavior (source not in this repo, public citro3d), [I] inference.
Rules for the agent: no feature toggles, no stubs, one row per commit, each row 100 changed lines or less. Do not restore the combiner-flush skip or the texture-bind skip (HANDOFF.md:11).

## 0. Starting numbers (console O3DS, yard, build 1175)
- Wall 69.3 ms, gbi walk 34.3 ms, tex-op streak 18.0 ms [M console_numbers.md, console_walk_split.md].
- fl tex: 281.9 closes per frame, 207.7 empty, 8.39 ms [M]. Empty close: 2 svc (the timer), about 2.6 us [M diag_ec2.md]. So about 0.5 ms is empty closes and about 7.9 ms is about 74 non-empty closes: about 106 us each [I arithmetic].
- Texture hits 86 per frame, 16.7 us each, 1.4 ms. Misses 1.6 per frame, 1.0 ms each [M].
- HANDOFF.md:15: about 65 closes per yard frame are a real texture change [M build 1141]. So at most about 9 of the 74 non-empty closes can merge [I]. The main lever is the cost per close, not the count.

## 1. What a non-empty close does (pm_gpu_flush_at -> flush_impl -> flush_draw)
Order of calls in one close, with file:line [C pm_gpu.c]:
1. pm_gpu_flush_at 2100: class lookup, pm_ticks pair around flush_impl (phase on) [C 2114-2118].
2. flush_impl 458: rect batch swaps combine/rect state, calls flush_draw [C 458-469].
3. flush_draw 709: early-outs (empty, nocol, failed texture) [C 712-728]. Off-screen snapshot [C 730-735].
4. Phase: PH_PUSH(PH_GPU), PHF_PUSH(PH_GBIND) [C 737-738]. Each push and pop reads pm_ticks (pm_phase.c:199-213). One close reads 4 ticks with phase on and 10 with phasegbi fine timing [C]. At 1.3 us per tick that is 5 to 13 us per close [I].
5. Raw batch (graw) only: program bind and attr info when the program changes [C 746, 787]. Uniform sets uvso/cmul/cadd/zmix/fx with memcmp shadows [C 747-763, 788-791]. Palette: per matrix, Mtx_Multiply when pcomp_gen is old, then C3D_FVUnifMtx4x4 (+3 FVUnifSet for pm5) when the shadow sh_pal_ok/sh5_pal_ok is not valid [C 765-783, 792-808]. pm5 clears every pm4 shadow and pm4 clears every pm5 shadow [C 765, 786]. A CPU draw clears pal 0 and 1 of both [C 829].
6. Projection uniform: skipped when equal on CPU draws (dr1_skips) [C 812-818]. Always sent on raw draws (proj_ok = !graw) [C 817].
7. C3D_GetTexEnv(0) on every draw [C 823]. [L] C3D_GetTexEnv sets the dirty flag of that stage, so stage 0 goes to the command list again at the next draw, even when the TEV key is equal.
8. Texture unit 0: pm_tex_bind (pm_tex.c:1302) skips filter/wrap/bind when tb_last, filter and wrap are equal, except a staged slot (slot_draws_stage), which binds every time [C pm_tex.c:1314-1326]. grab_bind and snap0 bind every time [C 824, 827]. Unit 1: pm_tex_bind_aux binds every time, no shadow [C pm_tex.c:1356-1360]. Unit 2 (noise) binds every noise draw [C 834]. tex1 uvx uniform set every tex1 draw, no shadow [C 830]. [L] C3D_TexBind always marks the unit dirty.
9. TEV: pso_pkt_try for compiled draws [C 843], else TevKey memcmp, rebuild only on a different key [C 845-855] (PG3 O4, commit 348e2e8e9).
10. Fragment state: sh_depth_test, sh_depth_map, sh_early, sh_alpha_test, sh_scissor, sh_cull, sh_blend all have argument shadows [C 380-410, 860-918]. Stencil calls in zmark and decal paths have no shadow [C 863-865, 875, 958-967].
11. User clip shadow sh_user_clip [C 919].
12. Draw: raw: buffer info only when buf_cur != 2, C3D_DrawElements [C 924-933]. CPU: C3D_GetBufInfo on every draw [C 955], BufInfo_Add only when !pool_buf_ok [C 957-961], C3D_DrawArrays [C 976]. [L] C3D_GetBufInfo sets the buffer-info dirty flag, so the attribute buffer block goes out again on every CPU draw.
13. kick_check [C 443-456]: once per frame, gpu_flush_written (GSPGPU_FlushDataCache of vertex, ring and index spans [C 255-265]) and C3D_FrameSplit. This one close holds a GSP call and a command list submit. Its time is in pm_ph_kick_us and also in fl us of that class [C 449-452] [I].
14. [L] C3D_DrawArrays/DrawElements run C3Di_UpdateContext: every dirty group is written to the command list (program, attr, buffers, texenv stages, texture units, effect, uniforms by dirty register range). Then the draw registers.
Redundant when nothing changed [C+L]: items 7, 8 (aux, noise, uvx, grab), 12 (GetBufInfo), and the clears in item 5 when no register was written. Items 6, 9, 10, 11 already skip equal state. The cost of each redundant item is a few command words [I]. They do not explain 106 us alone [I]. The candidates for the large part are: palette multiply and upload on raw closes (item 5), the tick reads (item 4), the kick close (item 13), staged texture binds (item 8). Section 3 measures them before any edit.

## 2. Merge of draws on equal resolved state
Close sites in pm_gbi.c [C]:
- 3066 G_SETTILE tile 0 when not same_tile: flush, then pm_tex_set_wrap.
- 2329 ensure_tile when tile_bound(n) is false: flush, then pm_tex_set_wrap and pm_tex_load (the deferred load from SETTILESIZE, GP3 3212).
- 3115 G_LOADTLUT on a bound CI tile with a new palette.
- 1164 aux tile key change. 2798 texture filter change (class 1). 2407 box pattern (class 5). 3221 shaded sprite.
- pm_gpu.c 1932: raw batch closes on any uvso/vp/cmul/cadd/zmix/ct change.
Why the two reverted skips failed [C+M]:
- Bind skip (reverted in f971e5dc4): at a tile-swap close it took tex/tex1/i8 from tev_key and did not call pm_tex_bind. tev_key does not hold which texture is bound. The queued batch drew with the texture of an older batch: stale menu textures [M HANDOFF.md:11,19].
- Combiner-flush skip: kept `comb` at 42 because the plan bytes do change (23 layout misses, 19 mode changes) [M HANDOFF.md:11]. No gain, not a picture error.
Safe condition for a merge [I, from the code above]:
- The close order must stay: the flush runs before pm_tex_set_wrap / pm_tex_load change pm_tex state, because flush_draw binds the current pm_tex slot (pm_tex_bind reads cur, wrap_s, wrap_t, tex_filt [C pm_tex.c:1309-1318]).
- So skip the flush only when a side-effect-free lookup proves that the state after the op equals the state now: same C3D_Tex pointer (slot index and off-screen index), same wrap ws/wt after shift/mirror, same filter, same palette, same i8 flag, same tile_x0/tile_y0/tile_fx/tile_fy and same uvso. These are the outputs that pm_tex_bind, flush_tev_rebuild (i8_0) and the vertex UV code read.
- This is the same test pm_tex_bind already uses for its own bind skip (tb_last + filter + wrap, pm_tex.c:1314). That skip has run since DR1 with no reported garble [I]. The reverted skip compared the TEV key, not the texture.
- A staged slot (slot_draws_stage) and an off-screen slot that the batch also writes (snap0) never merge.

## 3. Sampled counters (row DC1, first)
- One sample per 8 non-empty closes (a counter in pm_gpu_flush_at, sample when (n & 7) == 0). About 9 samples per yard frame [I].
- In the sample only, flush_draw stores pm_ticks at 6 marks: start, after palette/uniforms (line 819), after texture binds (836), after TEV (857), after fragment state (919), after the draw call (980 or 933). One more mark around kick_check.
- Cost: 7 ticks x 9 samples x 1.3 us = about 0.08 ms per frame [I].
- Plain counts per frame (no ticks): non-empty closes, raw vs CPU closes, palette uploads (gv_palup_n exists), Mtx_Multiply count (RS_PCOMP exists), TEV rebuilds, real C3D_TexBind calls on unit 0/1/2, staged binds.
- Print under `phase`: `dcsplit f<n> n <closes> raw <n> pal <us> bind <us> tev <us> st <us> draw <us> kick <us> palup <n> pmul <n> tevr <n> tb0 <n> tb1 <n> tbs <n>`. us fields are sample sum x 8 per frame.
- Also read pm_ph_kick_us from the existing PI4 line and subtract it from fl tex before any per-close claim.

## 4. Rows (in order, each 100 changed lines or less)
| row | change | files | measure | expected |
|---|---|---|---|---|
| DC1 | Section 3 counters. | pm_gpu.c, pm_phase.c | dcsplit on Azahar yard, then console push (orchestrator). | Names the largest part of 106 us [I]. No speed change. |
| DC2 | Remove redundant dirty marks: C3D_GetTexEnv(0) only in the rebuild path (823 to inside the else at 853), C3D_GetBufInfo only when !pool_buf_ok (955), unit 1 shadow in pm_tex_bind_aux (reset in pm_tex_bind_reset), unit 2 bind once after sh_clear. | pm_gpu.c, pm_tex.c | dcsplit tev/draw/bind us, draws per frame unchanged, fl tex us. | A few us per close [I]. |
| DC3 | Only if DC1 shows pal largest: move the pal shadow clears to the register writes. Clear sh_pal_ok[0..1] in the proj upload branch (816, already) and where uvx/nzo are set (830, 835), not on every CPU draw (829). Keep the clears at program change (765, 786). | pm_gpu.c | dcsplit pal us, palup and pmul per frame. | Fewer palette uploads [I]. Size depends on DC1. |
| DC4 | Only if DC1 shows tick reads above 10 us per close: remove PHF_PUSH/POP and PH_PUSH(PH_GDRAW) in flush_draw (737-856, 922-935, 954-978). dcsplit covers that split. Keep PH_GPU. | pm_gpu.c | wall ms and fl tex us. W and gpu phase move between fields: compare wall. | 5 to 13 us per close [I]. |
| DC5 | Merge on resolved state (section 2). Add pm_tex_peek in pm_tex.c: the hit path of pm_tex_load without side effects (no stamp, no load, no epoch bump), returns the slot index or -1. In ensure_tile (2328) and G_SETTILE (3065): skip pm_gpu_flush and the set when peek slot == pm_tex_cur_idx(), off idx equal, the wrap after wrap_of equal, filter equal, tile_x0/y0/fx/fy equal. Then update ens_* only. | pm_gbi.c, pm_tex.c | fl tex n non-empty, draws per frame, fl tex us. | At most about 9 closes x 106 us = about 1 ms [I]. |
Order: DC1 first. DC2 next (independent). DC3 and DC4 only on DC1 evidence. DC5 last.
Per row: the shot must show the subject. If the measured quantity does not change in two shots, revert the row.

## 5. Tests per row (Azahar, PM_ROW=<row>; no statehash, no golden.py)
1. Yard: preset=yard phase shots shotmode=4:80 battlef=80, shot 40. Mario, ground, shadow and post office sign visible (as build3ds/lane-ec2/shots/0040.ppm).
2. Pause menu: text "see" and the Boots entry are readable, icons correct.
3. File select: panels and title text, no garbled or stale textures (the bind-skip failure in HANDOFF.md:19).
4. Intro: Mario house sign readable (the IA4 sign of diag_tex2.md).
Record per row in docs/handoff/diag_<row>.md: draws per frame (gpusplit), fl tex n / empty / us, dcsplit fields, kick us, the 4 pictures.
Tests 2 to 4 matter most for DC2 (unit 1 shadow) and DC5 (merge): they hit the paths that broke before.

# Row T1F: native per-triangle aux TEXEL1 window fold

## User facts
Trigger: nok11_det (cold det, nok_11 splash). First wrong frame: shot 0014 (frame 412, splash a) and 0015. Expected: byte-equal to the 1e915fa5b base copy in C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\cfb2b13f-6944-48de-8418-7b56f133c798\scratchpad\base_shots\nok11_det. fb4 differs: 0014 509 px (box 90,101-192,158), 0015 56 px. Shot 0016 also differs base vs base: record only.

## Goal
TRI-4 (671567666) deleted draw_tri_cpu. The ring path drops aux TEXEL1 triangles (tridrop tex1 2168 in the f500 window, gv_epoch_enter GVR_TEX1 clause, nd_geom.c:183). Draw them natively with the per-triangle window fold the CPU path did.

## Start state
Worktree build3ds/wt/t1f, branch t1f from fb4 eae1f1bc7. Copy assets/us and ver/us/build into it. fb4 runs FB-4 in parallel (pm_gpu.c, nd_model.c, nd_tmpl.c): do not edit those except the rect_log tridrop line. GM-3 (deletes vert_uv) waits for this row.

## Oracle plan
CPU path per triangle: dual_bind() ends with pm_gpu_set_tex1(t1_p) (nd_comb.c:549, resets uvx to the plain window map), then t1_fold(min/max of the triangle's tile-0 u, v) (nd_comb.c:554). A changed uvx flushes the open batch (pm_gpu_set_tex1, pm_gpu.c:3207-3211). The native close copies t1_s into d.uvx (pm_gpu.c:1400). The ring path runs dual_bind once per epoch (nd_geom.c:179) and never calls t1_fold.
1. gv_epoch_enter (nd_geom.c:183): keep the refusal only for !td[0].valid || !td[1].valid. For pm_tex_aux_off_idx() >= 0 set ep_t1f = 1 (cleared at the top of gv_epoch_enter), not GVR_TEX1.
2. nd_comb.c next to t1_fold: void t1_fold_tri(const unsigned ri[3], unsigned tss), prototype in nd_int.h:36. Per vertex: Vtx at pm_gpu_gring() + 16*ri[k]; st = tc * (tss>>16 or tss&0xffff) / 65536.f (gv_xf formula nd_geom.c:123-124); u = (st/32*ms - tile_x0 - tile_fx)/tw, v the same (vert_uv formula pm_gbi.c:234-247: shift_mul, use_t1, pm_tex_size); !nd_ws.tex_on: u = 0.5/tw. Then pm_gpu_set_tex1(t1_p[0..3]); t1_fold(umin, umax, vmin, vmax). Same float expressions and order as the CPU path (git show 671567666 for draw_tri_cpu).
3. gv_tri: when ep_t1f, call t1_fold_tri after the why<0 test (after gv_params_calc, ~215) and before the gappend test (~217). The flush bumps flush_gen, so a new batch opens. A texgen vertex (vert_litf texgen) keeps the refusal.

## Premise and its check
Premise: the 2168 drops are the aux clause, and the 0014-0015 difference is the missing fold. Check: add t1fold (count of t1_fold calls with changed=1) to the tridrop line in rect_log (pm_gpu.c ~3997). Fix build f500 line: t1fold > 0 and tex1 0. t1fold 0: the fold is not the cause, revert, hand back at once.

## Touch only
nd_geom.c (gv_epoch_enter, gv_tri), nd_comb.c (t1_fold_tri), nd_int.h (prototype), pm_gpu.c (tridrop line only).

## Done criteria
1. Build OK. nok11_det: tridrop tex1 0, t1fold > 0; shots 0014 and 0015 byte-equal to the base copy; 0000-0013 still equal.
2. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold byte-equal to the fb4 eae1f1bc7 shots (run them on lane t1f once before the edit if no copy exists; save copies in your scratchpad).
3. If the only remaining 0014/0015 difference is a 1-px edge inside box 90,101-192,158 (CPU vs GPU transform rounding): stop and hand back with the diff box; the orchestrator decides by eye. 2 tries per named cause, no third theory.
Affected scenarios: nok11_det, spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=T1F PM_LANE=t1f on every build.sh and bench call. No git stash. Never commit 3ds/src/map_restore_table.c or runs.jsonl edits by hand. Wait for runs in the foreground. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=T1F PM_LANE=t1f sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=T1F PM_LANE=t1f python 3ds/tools/bench.py run <bench> --row T1F --lane t1f --why "<text>"
- Shots: build3ds/azt/shots/<bench>/NNNN_*.ppm in the worktree; compare with cmp.
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget T1F

## At the end
Delete this row file in the commit that completes the row. Report: commits, gate results, diff counts.

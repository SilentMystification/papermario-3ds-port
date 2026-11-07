# Row RS3: RECT-3 cleanup and move, then ST-3 move (oracle plans, 2026-11-07, nds1 6486c47da)

## Goal
Two commits in one worktree, in order: RECT-3 (dead rect debug code out, CPU texture builders to nd_rect.c), then ST-3 (legacy combine code to nd_comb.c). No new refusal. Pixels: layout only. Starts after LS-3 (rec_* to nd_recs.c) merges into nds1.

## Start state
Worktree build3ds/wt/rs3, branch rs3 from nds1 after the LS-3 merge. Copy assets/us and ver/us/build into it. Line numbers are from 6486c47da: find symbols by name.

## RECT-3
Premise: TRI-4 and FB-4 left pm_gpu_fill_rect and pm_gpu_tex_rect with only the native path. A refused rect is dropped and counted in rect_c[4] ("cpu" in the rect line). rect_raw_ok refuses only for rr_why 1 and 5. Nothing increments rr_why 2, 3, 4 or 6.
Check first (0 starts): the rect lines of a nok11_det or tour10_det log on nds1 show cpu 0, and no rectwhy line has a non-zero field. If the check fails, hand back.
Delete (dead):
- rectdbg: dbg_snap, dbg_have, dbg_take, dbg_cmp (pm_gpu.c ~657-700), and the `if (rect_open && dbg_have)` test (~734). dbg_have is never set.
- rectalt (~3761). It drops every second textured rect. No scenario uses it.
- cb_regdump (~893), only if `grep -c "cb_regdump("` is 1.
- rect_raw (622, 736, 738, 3642, 3647). It is always 1 while rect_open, so fold it into rect_open.
- Fix the stale comments: rect_begin (3704), "CPU path" (3631-3633, 3721-3724, 3752), "cpu" (842).
- Keep the rr_why array and the rect/rectwhy log field names, because the benches parse them.
Keep (live debug tools): rectred, rectnomerge, rectsig, rectpost/rp_draw, rectreg, zmforce.
Move to nd_rect.c: rect_to_image (pm_gbi.c ~355, caller nd_rect.c:46), box_pattern_rect (~416, caller nd_rect.c:61), box_buf[128*128*4]. Prototypes stay in nd_int.h:40-41. tile_texel stays external.
Stop rule: a grep count that is not as stated means stop and report.
Gate:
- build; grep proof that no definition is left in pm_gbi.c; nm shows them in nd_rect.o; nd_host PASS.
- Byte-equal to nds1, cold det: spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300, super_text_pic (box_pattern_rect path).
- rect lines cpu 0, rectwhy 0.
- One texrlog run with why 2 > 0 to show that rect_to_image still runs (pause or battle).

## ST-3 (premise of plan_s10_path.md row C refuted: legacy is live)
Fact: sync_combine_work calls sync_legacy on every combine and sends pm_gpu_set_combine(leg_mode) when the plan is not used (nd_comb.c ~451, ~350). decide uses cc_legacy for the error el (nd_comb.c ~306-317). The native templates implement legacy modes 1-7 (nd_tmpl.c ~163-209).
Check first: `grep -n "combine == [1-7]" 3ds/src/nd_tmpl.c` is not empty, and sync_combine_work still calls sync_legacy.
Move (pure move, D12): set_leg, leg_mode, sl_const, sl_env, sl_cc, sync_legacy, comb_mode5, cc_legacy (with its static CcIn helpers) from pm_gbi.c to nd_comb.c. Fix the externs (nd_comb.c ~21-22, 42-46; nd_int.h ~269). In the same commit, delete the "deleted once the CPU batch is gone" sentence in plan_s10_path.md row C and in row_MOVE.md.
Gate: build, nd_host PASS, the 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015).
Not in this row: an "always plan" combine (changes pixels where d->use is 0). That needs a separate design row with a census of el/ep.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=RS3 PM_LANE=rs3 on every build.sh and bench call. Budget 8 starts, reset once if needed. No git stash. Wait for runs in the foreground. Base shots: run the gate benches once on lane rs3 at the start commit and copy them to your scratchpad before the edit.

## At the end
Delete this row file in the ST-3 commit. Report in 5 lines.

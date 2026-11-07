# Row MDL5: model records fully native (why 6 to 0), then delete the walker geometry fallback

## Start state (nds1 099eaf6f6)
MDL-4 census (ndm fb at f300, source aa08821b1): mac_01 1, iwa_10 2748, sbk_56 0, kmr_03 battle 2453 (f600 0). All why 6: nd_model.c:261 nd_model_state (combine 8 with noise; tex1 with no ctx unit 1; pm5 gv->fxon with tex1) and pm_gpu.c:3253 model_draw (tile 1 bound, no unit 1: aux off-screen). pm_gpu_model_fb_cached and rec_run `if (!nat) rec_dl(...)` remain (S10 needs them gone).

## Oracle plan MDL-5 (2026-11-07)
Native forms in nd_model_draw, reusing the nd_raw_native code through shared helpers (no second implementation):
- a. combine 8 with noise: key noise bit + unit 2 (noise texture words, nzo uniform) as FB-2's nd_raw_native noise path; shared helper nd_unit2_fill(NdDraw*) in pm_gpu.c used by both.
- b. tex1 with no unit 1: take unit 1 from the live state after rec_apply: ensure_tile, dual_bind (MDL-1b v2 rule; pm_gbi_bind_tile1 for every record, not only R13), then pm_tex_native_unit1 and uvx = t1_s; shared helper nd_unit1_fill.
- c. tile 1 = aux off-screen image: live slot words; the snapshot only when the record draws into that slot (off_active && aux_off_idx == off_cur; the mac_00 rule). Same helper as b.
- d. pm5 with tex1: waits for FB-3 (pm5.v.pica outtc1 + uvx, per-entry pm5 palette). Then the b/c path with use5.
Census by producer: add `mdlfb f<n> <prod or list addr> sub <a|b|c|d> <count>` (top 8 per window) to the mdlw line; run iwa_10, kmr_03 battle, sbk_56 once each (cold det) and write the list here.
Picture reference: a `nomodelnat` debug switch (rec_run skips pm_gpu_model_native) if absent, deleted in MDL-5e. Byte-equal native vs nomodelnat for iwa_10, battle_start (all shots), sbk_56, mac_01 (cold det).
Rows (each gated by ndm fb for its sub-case = 0 at mac_01, iwa_10, sbk_56, kmr_03 battle, cold det, plus the byte gate):
- MDL-5a: sub-case b. MDL-5b: sub-case c. MDL-5c: sub-case a. MDL-5d (after FB-3): sub-case d.
- MDL-5e delete: pm_gpu_model_fb_cached, the why 6 neg-entry store (nd_mc_store_neg), rec_run `if (!nat) rec_dl(...)`, nomodelnat; keep a counted refusal only for a measured remaining case (then this row does not run). Gate: ndm fb 0 on the four scenes plus golden_multi and tour10; mac_01 byte-equal to the pre-row build; nm shows no rec_dl call from rec_run.

## Rules
Own worktree build3ds/wt/mdl5, branch mdl5 from nds1; copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). Cold det runs for byte gates; a missing or short shot file is the harness (rerun once). No attribution lines, never commit 3ds/src/map_restore_table.c, no git stash, keep Azahar open. Budget: -ResetBudget MDL5 at the cap. Delete this file in the MDL-5e commit.

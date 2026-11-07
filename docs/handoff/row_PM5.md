# Row PM5: lit and fogged (program 5) draws go native

## Goal
Battle effect_renderUI draws 1800 walker batches per 300 frames (wcen_battle, why pm5: g_use5 batches, program 5 with lighting and fog; graw_is_pm4 refuses them in nd_raw_native). Make them native, then delete the citro3d pm5 path. Plan: docs/handoff/plan_walker_end.md rows PM5-1 to PM5-4 and decisions D7 (reuse the prog5 register layout, no new shader), D9.

## Start state
nds1 at 113f8a243 (RECT-1b and RECT-2 census merged). Work in worktree build3ds/wt/rect, branch rect: git rebase nds1 first (or a new worktree build3ds/wt/pm5 -b pm5 nds1). Copy assets/us and ver/us/build if new. Build there, azt -Build <worktree 3dsx>.
The super block agent edits nd_tmpl.c tev_op_* in the shared folder: you own only the program/attr/uniform build parts of nd_tmpl.c, nd_emit.c, and in pm_gpu.c graw_is_pm4 and the g_use5 block of flush_draw. Do not change tev_op_*.

## Rows (one commit each)
PM5-1 TEMPLATE: NdTmplKey gets use5; nd_tmpl.c builds prog5, attr4 and the pm5 uniform block (lflag, fogu, light table, 7 registers per palette entry), same layout as the citro3d g_use5 path. Host test first: nat_host_test emitted words vs an ndref capture of one pm5 batch (wcen_battle effect_renderUI).
PM5-2 RAW: graw_is_pm4 accepts g_use5 for the native close. Gate: wcen_battle effect_renderUI wd 1800 to 0 (why pm5 0); battle shots right by eye (battle_start, battle_menu_bar); spot_mac01_pic byte-equal.
PM5-3 MODEL: nd_model_state drops why 4 (gv->fxon to the pm5 template). Gate: ndm why 4 = 0 at iwa_10 (one ndmcheck run), by eye, spot_mac01_pic byte-equal.
PM5-4 DELETE: the g_use5 citro3d block of flush_draw, sh5_* and u5_* without callers.
Hand back with the branch head after PM5-4 or a failed gate.

## State (2026-11-06)
Branch pm5 head 64000207d (worktree build3ds/wt/pm5): PM5-2 done (program change clears the native shadow; pm5 with tex1 or noise refused and named; no pm4 uv row on pm5 draws). battle_start_pic, super_text_pic (43 frames), mac_01 f200-202 identical native vs pm5off. wcen_battle census not read: phase census stall after the LOG5 merge (row LOGFIX). Not merged yet.
PM5-3 route (agent finding, accepted): (1) NdDraw carries the pm5 rows: per palette entry the 3 normal rows (pal_ntab[gm] at the producer), a use5 flag, and the fx block (lflag, fogu, light table) per draw; (2) the native submit (pm_gpu.c ~1163) does the pm5 upload from the NdDraw fields when use5 (same calls as pm5_upload, values from the draw, not walker globals); (3) nd_model_state drops why 4 for fxon only, keeps it where no normal rows exist; (4) gate: ndmcheck iwa_10 why 4 to 0, spot_mac01_pic byte-equal, fog and lit maps by eye.

## Oracle plan PM5 (2026-11-06, Opus; after PM5-2 picture fail on branch pm5 dad2564d3: census 1800 to 0, battle_start_pic dark square, label missing, glow)
Diff dad2564d3 vs 2206cd0a0, as read:
- pm5_upload stages the pm5 uniforms through citro3d. They go out in C3Di_UpdateContext inside nd_native_submit, so the order is right.
- The template words (nd_emit: TEV, units, depth) come from a key with no use5 and no fog. The tmpl "same" shadow then lets a pm5 draw reuse a pm4 template.
- NdDraw still gets has_uvx / loc_uvx (a pm4 register number) and the unit 1 words for a two-texture pm5 batch.
- Only the pm5 draw clears ndn_sh; a program switch itself clears nothing.
Causes, ranked:
1. TEV and fog do not match the walker's pm5 close. The citro3d pm5 path builds its TEV and fog registers from fog and lit state (tev_same key, fog mode in 0xE0, fog colour 0xE1, the fog LUT). nd_tev_build has no use5 or fog bit, so a pm5 batch gets the pm4 stages and a pm4 0xE0. This fits the glow at the enemy and the dark square.
2. pm4-layout per-draw writes land inside the pm5 uniform file. has_uvx writes u4_uvx (not gated by no_uni), and with tex1 or noise the unit 1 / nzo words are written too. Each overwrites a pm5 palette or light row. grange refuses pm5 with tex1; nd_raw_native does not.
3. State left behind after a pm5 draw (the missing Goomba label). C3D_BindProgram(prog4) on the next pm4 draw re-sends citro3d's stale uniform array over registers that ndn_sh may think are valid (pal entries of a RECT_GM or palette-0 draw). The tmpl shadow also does not know that the program changed.
4. Uniform values (lflag, fogu, light table order, 7-row palette stride, ring flags for the GPAL5 layout): same code as the walker's pm5 block. Low.
5. Attribute layout: attr4 for both programs, same as the walker. Lowest.
Measurement (one switch `pm5reg=182`, two cold runs of battle_start_pic: the PM5 build, and the same build with the pm5 gate off in nd_raw_native, which is the walker's pm5 close):
- Reuse the RECT rd_sync/cb_regdump parser; if HEAD lacks it, re-add the rd_* hunks from scratchpad s6/rect1ab_plus_diag_mine.diff, debug only.
- For f182, log one line for every pm5 draw and for the 3 draws after each pm5 draw: producer, use5, fx bits (lit, fog, texgen), combine, tex1, noise.
- Then the effective registers: 0x80-0x8E, 0x96, the 6 TEV stages, 0xE0, 0xE1, 0xFD, 0x100-0x107.
- Then the whole float uniform file the shadow holds (96 rows, from 0x2C0/0x2C1), the bound program, and the output map 0x4F-0x56 and 0x64.
- Pair the lines by draw index (the census order is the same in both runs). The first draw and the first register that differ name the cause:
  - TEV or 0xE0/0xE1 differ on a pm5 draw: cause 1.
  - a pm5 uniform row differs and the row equals u4_uvx/u4_nzo or a unit 1 write: cause 2.
  - only the draws after a pm5 draw differ (uniform rows or tmpl words): cause 3.
  - lflag, fogu or light rows differ: cause 4.
Fix per outcome:
- Cause 1: NdTmplKey gets use5 plus the fog class (the same bits the walker's TEV key uses). nd_tmpl.c nd_tev_build copies the walker's pm5 TEV and fog rule (stage plus 0xE0/0xE1 and the fog LUT write). The tmpl shadow keys on the id, so pm4 and pm5 never share a template.
- Cause 2: nd_raw_native refuses use5 with tex1 or noise, named (CW_PM5 sub-code, exactly as pm_gpu_grange does). nd_emit gates has_uvx and the nzo write on !no_uni.
- Cause 3: in nd_native_submit, every program change (bound changes, either way) clears the whole ndn_sh: uni_valid, pal_valid, tmpl = -1, col_valid.
- Cause 4: copy the exact walker order from the flush_draw g_use5 block (pal_ntab rows, flush_light_tab) into pm5_upload.
Gate:
- battle_start_pic f182 byte-equal to the rect build (the pre-PM5 picture).
- super_text_pic identical.
- wcen_battle: effect_renderUI wd 0, why pm5 0 (except a named pm5+tex1 count, if cause 2).
- The fog map and the lit map from the CEN-1a why-4 table right by eye.
- mac_01 byte-equal (shared nd_native_submit, nd_emit, nd_tmpl).
- Then PM5-3 (nd_model why 4).

## Rules
CLAUDE.md rules 5, 7, 9. No toggles (debug switches allowed, removed in PM5-4), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Keep Azahar open (no -NoKeep). Status line in 3ds/logs/perf/status_PM5.txt before each start. Budget: reset and continue. Two tries per named cause, then the next code point. Delete this file in the PM5-4 commit.

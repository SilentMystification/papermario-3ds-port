# Row S8FB: index of the fallback deletions (NATIVEAPI S8; one census row, then one Haiku row per helper group)

(Oracle 5 refresh. The rows S8CEN and S8FB1.. are written from this index when S7 (EFX, ENT, MAP, MDL) is merged; until then
the fallbacks still catch real cases, such as off-screen targets in the menus.)

## Rule (user, 2026-11-12)
A fallback is deleted only after its counter is 0 in every scene of its group, in one census run of the merged main. The census
expectation of each deletion row is exactly the counters of the code it deletes: a counter the row does not touch keeps its
value; a row that expects 0 while its own edits leave a path is a row fault.

## Census sources (main 5c3865487; re-grep when the rows are written)
- `napihud fallback f<n> n <n> why <w>` (3ds/src/nd_direct.c:187-190, nd_direct_fallback; first 16 and every 64th): HUD rect
  (hud_element.c:565 hud_rect_run), the complex HUD quad (S5CHUDB why 40), later callers with their own why values.
- `fill fallback f<n>` (nd_direct.c:256, nd_fill_run: S1FILL, S5PQUAD) and `trect fallback f<n>` (nd_direct.c:312, nd_trect_run:
  S5TRECT, S5TIMG, S5TTL): first 16 only.
- The msg glyph fallback (NAPI3B msg_glyph_run, nd_glyph_can 0): census `gdir` / `gfb` (NAPI3B merge message).
- The stencil fallback (S1TSTEN: `tsten f<n> id <i> direct 0`).
- `partsrun f<n> v <v> old <o>` (nd_walk.c:417): `old` counts nd_parts_run (the runner that walks pre units as ops); nd_part_draw
  returning 0 runs nd_ops_run on the list (nd_walk.c:436, :484).
- Entity interpretation: the `entnat` lines (nd_ent.c:310-315: why 1-8, segwhy); ent_run_seg_interp.
- `scomp: def` (nd_scomp.c:129) and refused defs (st 2, recorded as ops by rec_ops: no line today; S8CEN adds a cold count line).

## Rows to write then (each Haiku, one helper group, exact deletions, one byte list of at most 2 scenes)
- S8CEN: one list of the census scenes (DIRECT byte set, battle_wheel_nowide_det, pause_tab_det_pic, msgbox_talk_det,
  msgbox_choice_det, fxpic_63) on merged main; adds the refused-def count line. Hands back every counter per scene.
- S8FB1 HUD (hud_rect_emit, hud_rect_run's walk, cplx_quad_run's op block); S8FB2 fill and trect walks (nd_fill_walk, the
  nd_trect_run op block); S8FB3 glyphs and stencil; S8FB4 particles (nd_parts_run, the nd_ops_run arms of nd_parts_run_v);
  S8FB5 entities (ent_run_seg_interp, ENT_GBI arm); S8FB6 scomp rec_ops (refused defs: every def must compile first).
- nd_direct_ready and the NULL decision: kept until S9DEL2 removes the walker state they read (an off-screen target is real
  state, not a fallback); S9DEL2 replaces them by the native target check.

## Stop
This file is an index: no coder runs it. The last S8FB row deletes it.

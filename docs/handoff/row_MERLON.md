# Row MERLON: Merlon's house black interior and the square crystal ball

## User facts
User (2026-11-09, O3DS XL): "Entering Merlon's house doesn't load the texture on the inside, it's just black, the door, crystal ball and
Merlon." "Merlon's crystal ball ... a square that punches through the outside when you interact with the door. When you're inside,
instead of a circle it's a square that renders an odd texture from somewhere else."

## Start state
Worktree build3ds/wt/MERLON, branch MERLON from main (hash in the first commit). PM_ROW=MERLON, PM_LANE=MERLON.

## Facts (oracle)
- Ball (confirmed by code): gfx_build_inside_crystal_ball (src/world/area_mac/mac_01/crystal_ball_gfx.c:91-124) draws the ball into
  nuGfxZBuffer with Z_CMP, prim 248,240,240 (word 0xFFBC, z 32734) = a round depth mask, then a 32x32 texrect with G_ZS_PRIM,
  prim depth 32734, G_RM_ZB_OPA_DECAL. The mask used pm_gpu_set_zmark (old B3b 8afc7a55f) on the CPU triangle path, which TRI-4
  (6715676663) deleted: pm_gpu_set_zmark (pm_gpu.c:2736) has no caller; ROLE_DEPTH triangles are refused at nd_geom.c:163 (GVR_ZMARK)
  and dropped at nd_geom.c:280-281 (tri_drop[1]); pm_gpu_gtrip refuses r_zmark (pm_gpu.c:2522) though raw_zmark_pass (pm_gpu.c:1211,
  :1323) supports it except g_use5 (:1253). So no mask: the decal passes everywhere (square, through the wall, zoomed frame copy).
  Wide on: crystal_ball_gfx.c:56 computes x in 320 space; the ball draws at 1.25x-40 (pm_wide_draw_x, cam_main.c:519).
- Interior black (not proven): the interior group MODEL_dr_in (rooms.c:164) shows after ROOM_UPDATE_ENTER_BEGIN; since TRI-4 every
  triangle gv_tri refuses is dropped silently (nd_geom.c:280). Other candidates: native model cache (MAPX-2 key/hit shroud mix,
  pm_gpu.c:2816-2817, 3147-3152; nd_mc_store reuse nd_model.c:577-600). BTLDIM ruled out.

## Plan
1. Check first (one Azahar start): scenario+bench merlon_in_pic (xfer_door_mac01 recipe: story ch1_start, A at Merlon's door,
   loglevel=debug; shots about 120 frames after the A; until `xfer2 door n1\b`). Read the `tridrop f...` lines (rect_log,
   pm_gpu.c:3463) before/after the entry: zmark > 0 confirms the ball; another column jumping names the interior refusal; both 0 and
   still black: rerun with nomodelnat (interior appears: model cache cause; else add texmisslog). If det shows a lit room, repeat
   without det (logic thread on).
2. Ball: nd_geom.c gv_state_why (:163) stops refusing ROLE_DEPTH when depth source is not G_ZS_PRIM; gv_epoch_enter (:168-176) after
   sync_render: if img_role(0) == ROLE_DEPTH && !(omode_l & (1u << G_MDSFT_ZSRCSEL)) call pm_gpu_set_zmark(1, 2*depth_of_word(
   word_5551(nd_ws.prim)) - 1); pm_gpu_gtrip (pm_gpu.c:2522) refuses only `gs.r_zmark && use5`; fix the stale comment.
3. Ball wide (only if the wide picture is off-centre): crystal_ball_gfx.c under #ifdef TARGET_3DS maps f20 with pm_wide_draw_x.
4. Interior: fix the code point the check names (lit/fog/tex1 native in gv_tri/gv_epoch_enter; a gtrip refusal such as decal+blend
   :2523; or nd_mc_store / the hit mix). Two tries per named cause.
hw_guidelines: one call per state epoch, no tables, hot loop unchanged; no toggles; no stubs.

## Done criteria
nd_host PASS; byte-equal spot_mac01_det, battle_start_f182_cold, nok11_det, mdl5_iwa10_det; memcheck OK; pictures (wide on and off):
the interior lit with Merlon visible, the ball round with the zoomed scene, the door from outside with no square through the wall.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no push, no attribution. Never commit 3ds/src/map_restore_table.c. DIRECT/STEREO/DOORHITCH edit
pm_gpu.c and nd_* in parallel: merge main before each step.

## At the end
Delete this row file in the completing commit. SendMessage main "MERLON ready <hash>".

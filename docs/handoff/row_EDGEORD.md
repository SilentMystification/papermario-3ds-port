# Row EDGEORD: cutout rims (bushes, leaves, sprites) blend with the background instead of the object behind them

## User facts
User (2026-11-09, after KMR11W): "The transparency on the border of some things like bushes and tree leaves is transparent to the
wrong thing. It shows the background instead of whatever object is behind it, resulting in weird looking transitions."

## Start state
Worktree from `sh 3ds/tools/wt_new.sh EDGEORD`, branch EDGEORD. PM_ROW=EDGEORD, PM_LANE=EDGEORD. One Haiku agent.

## Cause (shared oracle, about 85%)
- nd_tmpl.c:291 (KMR11W): edge-AA cutouts blend SRC_ALPHA for alpha > 0x1F; nd_tmpl.c:342 keeps depth write (TEX_EDGE has Z_UPD).
- src/model.c:1274 RenderTaskBasePriorities[RENDER_MODE_ALPHATEST] = 1000000 (ALPHATEST_ONESIDED and SURFACE_OPA the same):
  execute_render_tasks (model.c:5150-5163) sorts them with the opaque models, near first. NPC sprites (npc.c:2156-2203) and item
  entities (item_entity.c:1084) use ALPHATEST too.
- So a near bush draws first, its rim blends with the background then in the buffer and writes depth; the object behind, drawn
  later, fails the depth test on the rim. The N64 hides this with its coverage bits and the VI anti-alias filter at scanout; the
  3DS has neither.

## Fix (src/model.c only)
1. model.c:1274 [RENDER_MODE_ALPHATEST] and :1276 [RENDER_MODE_ALPHATEST_ONESIDED]: under `#ifdef TARGET_3DS` 9000000, else 1000000.
   The 3M+ group is sorted far first (descending), and 9M is above every XLU base plus distance, so cutouts draw after all opaque
   models, far first, before XLU. Comment EDGEORD (rims blend at draw time; the N64 VI resolves coverage after all drawing).
   Leave ALPHATEST_NO_ZB (0) and ALPHATEST_NO_ZB_BEHIND (4250000) unchanged. Do not change nd_tmpl.c.
2. Check by code that execute_render_tasks and the floor reflection path (model.c:5206+) need no change.
3. Only if the picture gate fails or console frame time worsens by over 1 ms: revert, then the second fix point: a two-pass cutout
   in nd_tmpl.c and the native model draw (pass 1 atest 0x7F, blend off, depth write; pass 2 atest 0x1F, blend on, depth write off).

## Gates (light)
- nd_host PASS.
- kmr11_cut (until `shot \d+ frame 261\b`): shots 0010/0011, the bushes in front of the fortress wall and door: rims show wall or
  door colour, no sky-blue or black halo; the window rims stay clean. Run the base first to show the halo (repro).
- Byte check against stored base shots: gameover_fps60 (no ALPHATEST draw expected).
- memcheck OK (+0 KB). Console (user): bushes in front of houses and NPCs, Mario behind a bush; frame time vs dbcb623ba.

# Row S9DEL2: delete the run-time walker state model (combine decide, render sync, TMEM and tile model, nd_rect, gv walker state)

Model: Sonnet (user decides).
- Why not Haiku: the state model files are shared with the host converter (S7CONV runs nd_comb decide, nd_render_state and tex_win
  at build time). The coder moves them under the host build (tools side) and deletes them from the 3DS runtime without breaking
  the converter.
- Why not Opus: deletion and a move.

## Blocker and base
S9DEL1 merged. `sh 3ds/tools/wt_new.sh S9DEL2`; PM_ROW=S9DEL2 PM_LANE=S9DEL2.

## Pieces (grep each; list the hits)
- nd_comb.c: decide, decide_miss, sync_combine, scm memo, sc_sig_now, comb_in_walker. Move the decision to the host converter
  sources and keep nd_comb_decide only there.
- nd_set.c: sync_render and the op table.
- nd_tex_set.c: set_tilesize, tex_win0/1, ensure_tile, ensure_tile_slow, tile_bound. nd_tex_bind_desc stays if the native bind
  uses it: it holds no N64 state. Check.
- nd_rect.c (nd_texrect, nd_fillrect).
- nd_geom.c: the gv walker part (gv_tri state checks, nd_gv_params per range). Native mesh draws stay.
- nd_mtx_set.c: the list matrix stack, if no native caller is left. Native records carry float matrices.
- The nd_ws fields that only these read (nd.h NdWs).
- src/sprite.c (oracle 4): spr_state_run and spr_flat_gfx's unshaded arm. The native sprite reads its texture from the record
  (pm_gpu_spr_build, spr_build_rare: raster and palette words), so the TLUT and texture block ops of spr_state_run feed only the
  walker tile model and the sprite fallback (nd_walk_spr). Delete them with set_tilesize, and delete the record (nd_rec_napi of
  spr_state_run). The SPR_SHADING_FLAG_SET_VIEWPORT arms (the vpAlt pass, spr_final_list, SprFinal*) are dead code: the flag is
  never set (include/enums.h:5223). Delete them in this row too.

## Gate (expected values)
- Full build, memcheck OK; nd_host PASS (the converter host test passes from its new place).
- The byte set of S9DEL1: every shot byte-equal.
- `grep -rn "sync_combine\|set_tilesize\|tex_win0\|ensure_tile_slow\|nd_texrect\|decide_miss" 3ds/src` returns nothing.
- hotset M1 per class: classes A, B, E, G near 0 (hand back the numbers). Budget 8. Console: C5 after S9GATE.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.

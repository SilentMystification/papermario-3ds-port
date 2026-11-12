# Row SHADEIMG: the shade colour of a shaded sprite drawn through imgfx (Bombette first blow-up frames)

(Model: Haiku. Bug: user fact (console, mim_11): on the first frame of Bombette's blow-up animation her colour is wrong, as if the
lighting does not apply. Code only.)

## Cause (oracle code read, main after BOMBLIT2; census bombette_mim11 f90-f100)
BOMBLIT2: every frame 2 native shaded sprite draws (shnat 2, memo hits) and dual_f 156,156,255; shade_mean (one per shaded bake)
runs 3 times on f90-f95 and 9 times on f96-f98: on the blow-up frames 6 more shaded draws exist that are not native sprite records.
src/sprite.c:414-417 and :462-473: when CurSpriteImgFX has SPR_IMGFX_FLAG_ALL the component has no quad and no sprite record (sprRec
stays NULL, :430) and draws through imgfx_appendGfx_component: walker ops. The walker bakes them (tex_shaded0, dual_on), but the
shade colour multiply is only applied while nd_spr_walk is 1 (nd_geom.c:64 gv_params_calc: `if (dual_on && nd_spr_walk)`), and
nd_spr_walk is set only around the sprite record fallback (nd_walk.c:153). So an imgfx-drawn shaded sprite gets the bake without
the shade colour (cm not multiplied by 156/255, 156/255, 1): the lit colour is missing on the frames that use imgfx (the blow-up).

## Fix
Bracket the imgfx draw of a shaded sprite with a native marker that sets nd_spr_walk on core 0, as G_PM_ND_GRABHOLD brackets a draw:
the marker op bumps gv_epoch (op_ctl, nd_walk.c:324), so the params are made again with the shade colour inside and without after.

## Start
`sh 3ds/tools/wt_new.sh SHADEIMG`. PM_ROW=SHADEIMG PM_LANE=SHADEIMG.

## Touch only
include/gbi_custom.h (G_PM_ND kinds, :229-244), 3ds/include/nd_gbi.h (after gPmNdGutter), 3ds/src/nd_target.c (nd_native_rec),
src/sprite.c (the imgfx branch of spr_appendGfx_component, :462-473), the shadecen print (one counter).

## Exact edits
1. gbi_custom.h: `#define G_PM_ND_SPRSHADE 4` with a comment (SHADEIMG: arg 1 opens, 0 closes: the imgfx draw of a shaded sprite;
   the walker multiplies the KZN04 shade colour inside) and the raw form `#define gPmNdSprShade(pkt, on)` written as gPmNdGrabHold
   (:244) with G_PM_ND_SPRSHADE.
2. nd_gbi.h: `#undef gPmNdSprShade` and `#define gPmNdSprShade(pkt, on) ND_GX(pkt, ND_W0_PMND(G_PM_ND_SPRSHADE, (on) ? 1u : 0u), 0)`.
3. nd_target.c nd_native_rec: `if (kind == G_PM_ND_SPRSHADE) { extern int nd_spr_walk; nd_spr_walk = (int)on; }` and, for the census,
   `extern unsigned shc[]; if (on && PMDL("shadecen")) shc[7]++;` (slot 7 is free: shc[0..6] are BOMBLIT's) and print it as `img`.
4. sprite.c imgfx branch: before `if (imgfx_appendGfx_component(...` add `#ifdef TARGET_3DS` `if (gSpriteShadingProfile->flags &
   SPR_SHADING_FLAG_ENABLED) gPmNdSprShade(gMainGfxPos++, 1);` `#endif`; after the if-block (before line 473's `}`) the same with 0.

## Gate (CLAUDE.md 2-3; no console run in this row)
- Build, memcheck OK; nd_host PASS.
- `bench.py base spot_mac01_det` and `bench.py base mdl5_iwa10_det`; one list `bench.py run bombette_mim11,spot_mac01_det,
  mdl5_iwa10_det --row SHADEIMG --why "SHADEIMG: shade colour for imgfx shaded sprites"`. Pass: bombette_mim11 shadecen `img` > 0 on
  f96-f98 and 0 on f90-f95; spot_mac01_det and mdl5_iwa10_det byte-equal against `$(git merge-base HEAD main)` (no imgfx shaded sprite
  there). A byte difference: stop and hand back the cmp lines. Budget: 5 Azahar starts.
- The user checks Bombette's blow-up colour on the console with the batch push.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output. Delete this row file in the completing commit.

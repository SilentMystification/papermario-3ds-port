# Row BOMBLIT2: the colour inputs of the native shaded sprite on the first blow-up frame (measurement, step 2 of BOMBLIT)

(Model: Haiku. Bug: user fact (console, mim_11): on the first frame of Bombette's blow-up animation her colour is wrong, as if the
lighting does not apply for that one frame. Code only: no shot reading.)

## Facts so far
BOMBLIT (merged 0399d827e, switch shadecen, scene bombette_mim11): from the C-down press (about f94) every shaded raster set is baked
(m1 equals ok), and the shade path checks a-d never fail (cond, nopal, loadfail, why3 all 0). The first blow-up frame is f96 (m1 9).

## Code read (oracle, main a8fb2277e)
The native shaded sprite (pm_gpu.c:3956-3990 pm_gpu_spr_build) takes its shade colour from the global dual_f (KZN04, written by
shade_mean in load_shaded_sprite, nd_tex_set.c:664-692) times gv.cm (pm_gpu.c:3980), and its TEV template and stage colours from the
sprite memo nd_model_state_spr (nd_model.c:424-454). The memo key w[0..20] (:426-428) has no shaded flag, no dual_on and no
rgb_scale source other than c->rgb_scale; a hit copies the stored template and stage colours (:440-447) and the fresh cm from c->gv.
Path e (a hit that takes a template or stage colour of another sprite state) and a dual_f left by another component cannot be told
apart by reading: the decomp list order of the blow-up sprite components decides which component's shade_mean runs last.

## Start
`sh 3ds/tools/wt_new.sh BOMBLIT2`. PM_ROW=BOMBLIT2 PM_LANE=BOMBLIT2.

## Touch only
The shadecen counters of BOMBLIT (3ds/src/nd_tex_set.c shc[], its frame print) and pm_gpu.c pm_gpu_spr_native / pm_gpu_spr_build,
nd_model.c nd_model_state_spr (counter increments only, under PMDL("shadecen")).

## Exact edits (no draw change)
Add to the per-frame shadecen print: `nat <ok> fb <n> why5 <n> sprmh <hit> sprmm <miss> shnat <shaded native draws> dmin <r,g,b>
dmax <r,g,b> smean <shade_mean calls>`: nat and fb at pm_gpu_spr_native (:3902-3904), sprmh and sprmm at nd_model_state_spr (:437,
:450), shnat and the dual_f min and max (dual_f[0..2] * 255, ints) at pm_gpu.c:3980 when shaded, smean at shade_mean.

## Gate (measurement)
Build, memcheck OK. One Azahar start: `bench.py run bombette_mim11 --row BOMBLIT2 --why "colour inputs of the native shaded sprite
on the first blow-up frame"`. Hand back the shadecen lines f90 to f100 as printed. Stop after the hand-back.

## At the end
Delete this row file in the completing commit.

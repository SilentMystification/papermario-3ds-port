# Row STEREOCRASH2: list readers stop at G_ENDDL (nd_op_unit); the HC4e2b wild walk (Haiku)

## Cause (oracle batch 6)

**Cause.** The HC4e2b edit (9e5439d4f) made the game-side list readers stop on `NDO_END`. But `nd_op_xlate` (3ds/src/nd_op.c:20-81) has no `G_ENDDL` case: 0xDF falls to `default`, which leaves `out[0]=w0` raw and returns 0. So `cmd` is 0xDF, never `NDO_END` (0x2E), and the loop never sees the end of the list. nd_ent.c:107-111 (`ent_unit`) and nd_shape.c:40 already say this ("nd_op_xlate has no G_ENDDL arm"). HC4e2b did not apply that rule at its own sites.

**Where it overruns.** imgfx_load_anim, src/imgfx.c:1313-1342, the same code on 2c29fb04a and on main. `do { w0 = gfxBuffer[j++].w0; nd_op_xlate(w0, w1, t); ... if (cmd == NDO_VTX) gfxBuffer[j-1].w1 = ((w1 - keyframesOffset)/3)*4 + vtxBufs[i]; } while (cmd != NDO_END);`
- The `t[2]` buffer passed to nd_op_xlate is a stack local, 2 words. It is not the overrun. The gdb PC sits in nd_op_xlate because that is the next call in the loop.
- The loop walks past the end of `gfxBufs[i]` (heap_generalHead 0x00d4f400, then the sprite heap, then .bss). It stops only at an 8-byte-aligned word whose top byte is 0x2E.
- On each word whose top byte is 0x01 (G_VTX) or 0x36 (NDO_VTX), it rewrites the next word.
- At 0x01041968 the walk reads `w0 = RenderTaskLists[2]`, a pointer with top byte 0x01 (G_VTX). It then rewrites `w1`, which is `RenderTaskListIdx` at 0x0104196C.

**Map and register evidence:**
- r4 0x1041970 is `&gfxBuffer[j]` after `j++`.
- `w1=1` in the bt is the old index.
- r9 0x55555556 is the divide-by-3 constant.
- r5 is `ImgFXAnimHeaders[12]`.
- The new value 0xC8BEBC is `(1-kf)/3*4 + vtxBufs`. The earlier 0x01D46D04 fits the same formula.
- The .bss just before RenderTaskLists is `DepthCopyBuffer` (u16[16], 0x01041940). It is bounded (pm_depth.c checks `id`) and is not involved.

**Same bug at every HC4e2b site:**
- src/animator.c:912/919: the `for(;;)` scan has no bound.
- src/animator.c:966: the copy.
- src/model.c:4499: `mdl_local_gfx_update_vtx_pointers` copies with no bound, so it overruns the heap.
- src/entity_model.c:266 and :307: bounded at 2048, but the counts are wrong.
- src/effects/energy_orb_wave.c:259: debug log only.

**Not only stereo.** Every imgfx mesh-anim load, animator copy and local-vertex model copy walks wild memory. Whether it crashes depends on what lies after the buffer. Stereo (STEREOFX1 static arrays, a different heap state) moves `RenderTaskListIdx` into the walk's path.

**This is not the user's original crash.** The user's build 8085ef1d0 does not contain 9e5439d4f (checked with `git merge-base --is-ancestor`). The Watt dark-room slider crash and the first Merlee crash fit STEREOFX path 1 (malloc on the first stereo frame), which STEREOFX1 fixed. This bug is a newer regression that the repro build hit. After the fix, the user must retest on a new push.

**Row STEREOCRASH-FIX (Haiku):**
- Start: `sh 3ds/tools/wt_new.sh STEREOCRASH2` from main. Set PM_ROW=PM_LANE=STEREOCRASH2.
- Edit 1, 3ds/include/nd_op.h, after the prototype at line 137 `int nd_op_xlate(...)`, add:
  `static inline void nd_op_unit(uint32_t w0, uint32_t w1, uint32_t t[2]) { if (nd_op_typed(w0 >> 24)) { t[0] = w0; t[1] = w1; } else if ((w0 >> 24) == G_ENDDL) { t[0] = NDO_W0(NDO_END, 0); t[1] = 0; } else nd_op_xlate(w0, w1, t); } /* the typed unit of a list word; G_ENDDL -> NDO_END (nd_op_xlate keeps it raw) */`
  If `G_ENDDL` is not visible there, include `"PR/gbi.h"` the same way nd_op.c gets it (via ultra64.h).
- Edit 2, 3ds/src/nd_ent.c:108-112: delete `ent_unit` and replace its callers with `nd_op_unit`.
- Edit 3: replace `nd_op_xlate(` with `nd_op_unit(`, same arguments, at src/imgfx.c:1319, src/animator.c:912, :919, :966, src/model.c:4499, src/entity_model.c:266, :307 and src/effects/energy_orb_wave.c:259. Do not touch nd_op.c, nd_rec.c, nd_recs.c, nd_walk.c or nd_shape.c: they handle G_ENDDL before the call.
- Build: `PM_ROW=STEREOCRASH2 PM_LANE=STEREOCRASH2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`
- Gates:
  1. `python 3ds/tools/bench.py run nd_host --row STEREOCRASH2 --why "nd_op_unit"` gives PASS.
  2. `bench.py run merlee_stereoat --row STEREOCRASH2 --why "imgfx list end"` reaches "merlee: darken done" with no exception or crash page.
  3. Byte check, because the animator and model copies feed the walker: `bench.py base spot_mac01_det`, then `bench.py cmp spot_mac01_det --row STEREOCRASH2` gives 6/6.
- Stop: 3 Azahar starts. If gate 2 fails, hand back the crash PC and LR and the log tail.

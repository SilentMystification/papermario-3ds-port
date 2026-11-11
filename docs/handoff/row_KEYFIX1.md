# Row KEYFIX1: the matrix stack pointers out of the model cache key (moving and billboard models)

(Model: Haiku. Base: main 436abd053. Runs together with KEYMISS and BATUI: no shared file. The agent reads only this file and CLAUDE.md.)

## Cause (oracle code read, main 436abd053)
- Key A (nd_mkey_a, pm_gpu.c:3200-3211) hashes record word 5 (R2 w1 = model->finalMtx, src/model.c:1562) and word 17 (R8 w1 = the
  billboard matrix pointer, model.c:2196). While a model moves, finalMtx is a new slot of the frame's matrix stack
  (`&gDisplayContext->matrixStack[gMatrixListPos++]`, model.c:3042 and :3054; the display contexts alternate); a static model uses
  `&model->savedMtx` (:3046), flagged by bit 0x100 of word 3 (model.c:1541). camera->mtxBillboard is a stack slot of this frame
  (cam_main.c:221). So a moving or billboard model misses every frame, and nd_mc_store (nd_model.c:646) replaces its entry each time.
- A hit does not read these pointers: rec_run runs rec_matrix (nd_recs.c:65-70, both matrix loads) before pm_gpu_model_hit, the hit
  rebuilds palette 0 from the combined matrix (pm_gpu.c:3563-3564), the group mask is tested fresh (model_vis_same :3385), and the cull
  sense gv_wind comes from the post matrix only (pm_gpu.c:187-192). Only a lit or fogged draw (use5: the pm5 normal rows of the matrix,
  nd_mc_store nd_model.c:670) depends on the matrix; its guard e->mver (pm_gpu.c:3535) is pmMtxVer, which moves only when the matrix
  is saved (model.c:3038), not while the model moves or the camera turns a billboard. Such an entry must hit only on a saved matrix.

## Start
`sh 3ds/tools/wt_new.sh KEYFIX1`. PM_ROW=KEYFIX1 PM_LANE=KEYFIX1 on every build and bench call.

## Touch only
3ds/src/pm_gpu.c: nd_mkey_a, pm_gpu_model_key, pm_gpu_model_hit. No other file or function.

## Exact edits (pm_gpu.c)
1. nd_mkey_a, in the loop after `else if (shr && j == 8u) v &= 0xffffff00u;` add:
   `else if (j == 5u && !(rec[3] & 0x100u)) v = 0; /* KEYFIX1: a moving model's finalMtx is a matrix stack slot of this frame */`
   `else if (j == 17u) v = v != 0u; /* KEYFIX1: the billboard matrix is a stack slot of this frame: only its presence */`
2. pm_gpu_model_key, after `k->w[3] = 0;` add the same rule on the full key (mkchk compares the full words, nd_model.c:483):
   `if (!(rec[3] & 0x100u)) k->w[5] = 0; k->w[17] = rec[17] != 0u; /* KEYFIX1: as nd_mkey_a */`
3. pm_gpu_model_hit, after the line `if (e->mver && e->mver != ...` (:3535) add:
   `if (e->mver && !(((const uint32_t*)(uintptr_t)k->rp)[3] & 0x100u) && !(((const uint32_t*)(uintptr_t)k->rp)[6] & 256u)) { ndn_rng++; pm_xf.mdl_rng++; return 0; } /* KEYFIX1: a lit or fogged entry (pm5 rows of its matrix) hits only on the saved matrix; IGNORE_MATRIX records load none */`
   A refused hit goes to rec_miss (tried 1), which stores the draws in place (nd_mc_store :651): no pool growth.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- `PM_ROW=KEYFIX1 PM_LANE=KEYFIX1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row KEYFIX1 --why "model key words 5 and 17"`: PASS.
- `bench.py base spot_mac01_det` and `bench.py base mdl5_iwa10_det`; then one list
  `bench.py run spot_mac01_det,mdl5_iwa10_det --row KEYFIX1 --why "model cache hits of moving and billboard models draw the miss bytes"`;
  then `bench.py cmp spot_mac01_det --row KEYFIX1` and `bench.py cmp mdl5_iwa10_det --row KEYFIX1`: 6/6 byte-equal each.
- `bench.py run hotset_size --row KEYFIX1 --why "pm_gpu_model_hit grew one line"`: PASS (a stale hot_order name from this change:
  rename it in the hot order file, rebuild, run again once).

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the gate results, the memcheck line.

# Row KEYFIX2: the post list pointer out of the model cache key

(Model: Haiku. Waits: KEYFIX1 merged (same functions in pm_gpu.c) and the KEYMISS census merged with word 23 in a keymissw line
(count > 0 in any of the 4 scenes). With no word 23 count: the orchestrator deletes this row. Branch from main after both merges.)

## Cause (oracle code read, main 436abd053)
- Record word 23 (R11 w1) is the post custom gfx list: a range of this frame's stream (nd_cgfx_rng, src/model.c:2247, :4307
  nd_rec_mark), a new address each frame. Key A (nd_mkey_a, pm_gpu.c:3200) hashes it, so a model with a post list and no pre list
  misses every frame (a pre-list record is never looked up, nd_recs.c:315).
- The post list runs after the draw on a hit and on a miss (nd_recs.c:296, :326): the cached draws do not read it. Its only effect on
  the draw is the refusal test nd_r11_ok(rec[23], 0) (nd_model.c:328, why 3: since MDL-5e a refused model is not drawn). The hit must
  keep that test, so a post list that becomes unsupported is refused as before.

## Start
`sh 3ds/tools/wt_new.sh KEYFIX2`. PM_ROW=KEYFIX2 PM_LANE=KEYFIX2 on every build and bench call.

## Touch only
3ds/src/pm_gpu.c nd_mkey_a and pm_gpu_model_key; 3ds/src/nd_model.c (one new function after nd_r11_ok); 3ds/src/nd_recs.c rec_run and
one prototype line.

## Exact edits
1. pm_gpu.c nd_mkey_a, after the KEYFIX1 line `else if (j == 17u) ...` add:
   `else if (j == 23u) v = v != 0u; /* KEYFIX2: the post list is a frame stream range: only its presence */`
2. pm_gpu.c pm_gpu_model_key, after the KEYFIX1 line add: `k->w[23] = rec[23] != 0u; /* KEYFIX2: as nd_mkey_a */`
3. nd_model.c, after the function nd_r11_ok (ends near :307) add:
   `int nd_r11_post_ok(uint32_t addr) { return nd_r11_ok(addr, 0); } /* KEYFIX2: the hit keeps the post list test of the miss */`
4. nd_recs.c: after the line `extern int nd_r11_state(...` (:110) add `int nd_r11_post_ok(uint32_t addr); /* KEYFIX2 (nd_model.c) */`.
   In rec_run replace `if (!pm_gpu_model_hit(&mk, mce)) return rec_miss(q, &mk, mce, 0, 1, mi0);` by
   `if ((q[11].words.w1 && !nd_r11_post_ok((uint32_t)q[11].words.w1)) || !pm_gpu_model_hit(&mk, mce)) return rec_miss(q, &mk, mce, 0, 1, mi0); /* KEYFIX2 */`
   (tried 1: the matrix ran; rec_miss runs rec_apply and the refusal as before).

## Gate (CLAUDE.md 2; Azahar budget 6 of 8)
- `PM_ROW=KEYFIX2 PM_LANE=KEYFIX2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row KEYFIX2 --why "nd_model.c nd_r11_post_ok"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det --row KEYFIX2
  --why "post-list model hits draw the miss bytes"`; `bench.py cmp` of both: 6/6 byte-equal each.
- The census scene that showed word 23, alone: `bench.py run <that keymiss scene> --row KEYFIX2 --why "word 23 misses after the fix"`:
  its keymissw line has no ` 23:` entry.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the gate results, the memcheck line, the keymissw line.

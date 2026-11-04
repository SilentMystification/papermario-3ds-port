# diag_s1_1_picture: census_toad_shot change of S1-1 (3e6b19bc2)

Read-only diagnosis. No game source edited. Branch nds1 not switched.
Tags: CONFIRMED (measured or read in code), INFERRED (reasoned, not measured).

## 1. Result

- Verdict: NEUTRAL by the gate (PASS). Direction not proven. No fix applied.
- The 300 px change is texel-level on dark brick texels of the Toad Town wall. No object moved. CONFIRMED.
- The changed texels get brighter in 854 of 869 changed channels. CONFIRMED.
- The cause is the removal of the record replay path (dlc_learn, rec_try/replay, pm_tex_rec_use). After S1-1, the static DLC2 groups draw through the live walk (dlc_static_group, dlc_seg) every frame. CONFIRMED (code). INFERRED: the S0 census frame 200 used replay for groups after their first learn frame.
- Which picture is correct is not measured. No N64 or hardware reference pixel exists in the repo for this view. INFERRED: the replay texture binding is the source (see section 4).

## 2. Inputs

- Baseline: 3ds/tests/ndref/picbase_census_toad_0000.ppm (S0 build 3cdd0b542, lane base).
- S1-1 shot: build3ds/azt/shots/census_toad_shot/0000_0_8.ppm (S1-1 binary; the S1-2 run is 0 px equal to it, so this file is the S1-1 picture). CONFIRMED.
- region_diff (python -I 3ds/tools/region_diff.py on copies): 300 px (0.31 %), 42 regions, largest 27 px. Reproduced. CONFIRMED.
- Diff image (scratchpad only, not committed): C:/Users/Andrew/AppData/Local/Temp/claude/c--Users-Andrew-Documents-GitHub-papermario-3ds-port/bf7242ad-7188-4cf3-8ec3-af3ba8c281b2/scratchpad/diff_crop.png (base, new, diff; diff px red).
- Pixel dump script: scratchpad dump.py (same folder).

## 3. Pixels

- Top regions (x,y box, size, mean base to mean new, RGB):
  - (186,257)-(199,262) n 27: (10,13,1) to (26,17,3)
  - (108,83)-(110,90) n 20: (43,35,0) to (77,52,2)
  - (98,84)-(100,91) n 19: (71,47,1) to (100,64,2)
  - (186,185)-(188,190) n 18: (33,35,4) to (66,43,9)
  - (186,192)-(188,198) n 17: (43,27,5) to (68,49,11)
- Full list: 42 regions, all within x 98 to 206, y 70 to 310.
- Base colour of changed pixels: all in the dark buckets (channel value below 48). The changed pixels are 300 of 45072 pixels in those buckets.
- Channel ratio new/base (base at least 16): p10 1.10, p50 1.33, p90 2.16. Not one constant factor. Not a whole-triangle tint. CONFIRMED.
- Up 854 channels, down 15. CONFIRMED.

## 4. Code path

- Diff S0 to S1-1 (3ds/src/pm_tex.c, pm_gpu.c, pm_dlc.inc.c, pm_gbi.c). The removed code is record and replay code plus NatRec bookkeeping (nat_seg_add, nat_entry_state).
- pm_tex.c: removed pm_tex_rec_use, which called TOUCH(&slots[s]). This changes LRU stamps (evict_oldest_done). It changes eviction order only. It does not re-decode a texel. INFERRED that it does not cause the pixel change alone.
- pm_gpu.c: removed record words (rec_fw, rb_*, pm_gpu_rec_walk/ same/ idx). Record-only. The flush and TEV path is unchanged.
- pm_dlc.inc.c: DLC2 walk keeps dlc_seg and dlc_set_geom. Removed dlc_learn and the replay branch in dlc_try. The live walk is the path of the first (learn) frame for each group.
- Known replay defect: docs/handoff/diag_uc3a_bad.md (CONFIRMED in code). A record stores the physical address of the linear stage copy of a texture (FCRAM). The live frame stores the VRAM copy. The record key cannot tell staged from uploaded. A replayed draw can read a texture copy that differs from the live copy.
- A stale or staged copy gives texel-level differences with a brighter or darker value and no object shift. That matches section 3. INFERRED.

## 5. Fix

- Not a regression by evidence. No fix proposed.
- If the user decides the baseline (replay) picture is the target, the fix is a new row. It must restore the replay texture binding check (refuse a record while a bound slot is in its load frame, per diag_uc3a_bad.md section 1). This path is deleted in S1-1, so it is not a small row.

## 6. Open

- Measure the texel values in the two pictures at one changed pixel (for example 186,257) and the slot address the draw used. One quantity per shot.
- The mem line (PM_LINEAR_FIXED_KB) is still open for S1-6. Not part of this diagnosis.

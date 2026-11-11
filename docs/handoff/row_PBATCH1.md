# Row PBATCH1: a static effect list called many times per frame draws through the ring path (one draw per 8 matrices)

(Model: Haiku. Base: main. Edits 3ds/include/nd_int.h EntKey (two fields), 3ds/src/nd_ent.c ent_gdl, 3ds/src/nd_walk.c walk_call.
Disjoint from TILEWIN, PROFDISK, R13NOP, LOGX1-3. Reads this file and CLAUDE.md.)

## User facts (console fxtest, O3DS, cost above the 32.8 ms base)
snowfall +14.1 ms (120 draws), embers +10.5 (63), energy_shockwave +7.2 (87), effect_3D +5.0 (58), pink_sparkles +4.7, shimmer_burst +3.4.

## Cause (oracle code read)
- Each particle is gSPMatrix + gSPDisplayList of one static effect list (snowfall.c:207-218, embers.c:211-220, shimmer_burst.c:236-243).
  walk_call (nd_walk.c:221-243) gives a non-map list to ent_gdl (nd_ent.c:332) -> ent_run (:389-412): per call the list's state words
  replay (ENT_GBI :403) and each segment is one native range (ent_seg_native -> dlc_seg -> pm_gpu_grange, pm_gpu.c:3020). A static batch
  extends only under the same matrix slot (grange_batch_open, :3033: "same PSO inputs and matrix slot"), so every particle flushes a
  draw, and the native draw resets the walker shadows (sc_ok 0), so the next call's state words sync again.
- The compiled vertices carry no per-instance matrix index, so calls of one list cannot share a native draw. The ring path (the walker
  ops: nd_ops_run -> nd_geom_vtx -> gv_tri) stores a matrix id per vertex and batches up to 8 matrices per draw, and its state words skip
  while equal (nd_op_same). CPU cost: one vertex copy and one triangle setup per triangle (snowfall 935 tris per frame).
- Rule: a list that was called 4 or more times in the previous frame (a particle list) takes the ring path for the whole frame.

## Start
`sh 3ds/tools/wt_new.sh PBATCH1`. PM_ROW=PBATCH1 PM_LANE=PBATCH1 on every build and bench call.

## Exact edits
1. nd_int.h EntKey (:334-347): add `u16 calls, pcalls; u32 cf; /* PBATCH1: calls in frame cf, calls in the frame before */`.
2. nd_ent.c ent_gdl, after the `if (k->useseg) { ... }` block and before the ENT_DYN test:
   `{ const u32 f = pm_frame_count(); if (k->cf != f) { k->pcalls = k->cf + 1u == f ? k->calls : 0; k->calls = 0; k->cf = f; } if (k->calls < 0xffffu) k->calls++;`
   `  if (k->pcalls >= 4u) return -1; } /* PBATCH1: a particle list (4+ calls last frame): the ring path batches 8 matrices per draw */`
   Update the comment of ent_gdl: returns 1 drawn, 0 not drawn, -1 ring path wanted (the caller skips dlc_try).
3. nd_walk.c walk_call: `int eh = ent_gdl(next); NDT_E(te, NDT_ENT); if (eh) return;` becomes
   `int eh = ent_gdl(next); NDT_E(te, NDT_ENT); if (eh > 0) return; if (eh < 0) ring = 1;` with `int ring = 0;` declared before the
   `if (ent_skipgen)` line, and the dlc test `if (!ow && dlc_on) {` becomes `if (!ow && dlc_on && !ring) {`. No other caller of
   ent_gdl uses its value as a bool (grep `ent_gdl(`; a caller other than walk_call: make it test `> 0`).

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PBATCH1 --why "ent_gdl ring rule"`: PASS; `bench.py run hotset_size --row PBATCH1 --why "walk_call"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,fxcen_c3,fxcen_c6,fxcen_c7 --row PBATCH1 --why "particle lists on
  the ring path"`; `bench.py cmp spot_mac01_det --row PBATCH1`: write the byte-equal count (a repeated list in mac_01 may now draw
  through the ring: a diff is not a fail; write its pixel count).
- `python 3ds/tools/fxcen_report.py <the three fxcen LOG: copies> --label pbatch1`: ndraw per frame of snowfall (63), shimmer_burst (50),
  energy_shockwave (51), effect_3D (55), embers (105), pink_sparkles (124) is at most 1/4 of the FXCEN2 table, gvtri is higher. Write the
  six rows in the commit. The look and the console ms are the user's check (console fxtest push with the same chunks).

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the six rows.

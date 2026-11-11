# Row FILLA0: translucent draws without depth write skip alpha-0 fragments (alpha test > 0): same picture, less frame-buffer traffic

(Model: Haiku. Base: main after EDGEAA (same function nd_render_state, other lines). Edits 3ds/src/nd_tmpl.c nd_render_state (one
line at its end). Reads this file and CLAUDE.md.)

## User fact
fire_breath +17.4 ms per frame on the O3DS with 7 draws, 204 triangles, texture misses 0.05: the cost is fill. Fix without a look change.

## Facts (oracle code read)
- fire_breath (src/effects/fire_breath.c:160-223) is already one draw per puff: one mesh list with both tiles in one 2-cycle combine
  (tile 0 and tile 1 windows per animation frame, :210-211), render task mode RENDER_MODE_CLOUD_NO_ZCMP (:175: no depth test, no depth
  write, alpha blend). There is no second pass to merge and no depth work to drop.
- A blended fragment with alpha 0 leaves the frame buffer unchanged (src x 0 + dst x 1) but still costs the PICA a colour read and write.
  The puffs are large scaled quads whose texture is mostly transparent around the flame. The alpha test discards a fragment before the
  frame-buffer stage; nd_render_state leaves blended draws without an alpha test (`o->atest = -1`, nd_tmpl.c:294; R_ALPHA_TEST GREATER
  aref when on, :377).
- Equal output needs: the N64 alpha blend (o->blend: P = CLR_IN, A = A_IN, M = CLR_MEM, B = 1-A, :299), no depth write (an alpha-0
  fragment of a depth-writing draw writes depth today), colour on (not nocol), not the memory blend (memb). Then skipping alpha 0 changes
  no pixel. Other options read and rejected: merging passes (one pass already), tighter meshes (needs texture alpha hulls per frame: a
  build-time asset change, ASSETFREE study), filter changes (the PICA bilinear costs no fill).

## Start
`sh 3ds/tools/wt_new.sh FILLA0` after the EDGEAA merge. PM_ROW=FILLA0 PM_LANE=FILLA0 on every build and bench call.

## Exact edit (3ds/src/nd_tmpl.c nd_render_state, after the line `o->zcmp = ...; o->zupd = ...; o->decal = ...;` at the end)
`if (o->blend && o->atest < 0 && !o->zupd && !o->nocol && !o->memb) o->atest = 0; /* FILLA0: an alpha-0 fragment of an alpha blend`
` * without depth write changes no pixel: the alpha test (GREATER 0) drops it before the frame-buffer read and write (fire_breath puffs) */`

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; `bench.py run nd_host --row FILLA0 --why "render rule alpha 0"`: PASS (if nd_tmpl_test compares held words with
  atest -1 for these draws, stop and hand back the failing lines: the test needs the same rule as an exception, as KMR11W had).
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,fxcen_c3 --row FILLA0
  --why "same picture, alpha-0 fragments dropped"`; `bench.py cmp` of both maps: 6/6 byte-equal each (the pass gate).
- The console ms of fire_breath (fxcen chunk 3) is the user's check.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.

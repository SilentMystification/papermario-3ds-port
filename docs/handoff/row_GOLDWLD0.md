# Row GOLDWLD0: golden drec base and site coverage of src/world/area_mac/mac_01/crystal_ball_gfx.c (pilot of the device drec form)

Missing fact: no scene runs 90 % of the sites. 7 of the 20 active sites (the pre and post builders, B1 and B2 below) run only while
MODEL_mirrorball is on. Only the fortune event turns it on (src/world/area_mac/mac_01/npc/read_fortune.inc.c:142
`EnableModel, MODEL_mirrorball, true`; crystal_ball_gfx.c:228 turns it off at map setup). The only input switches are padscript
and story (3ds/src). No scene file reaches Merluvlee's fortune event. User decision: give the scene (a padscript path to a paid
fortune, or a story value and a position inside her house). Until then the row stops at gate 2.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 182ac5091)
- The file has 44 site lines: listcen build 13 and build_n64 31. N64DEL2 deletes the 31 N64-only lines and the 9 TARGET_3DS
  blocks, so it edits this file. This row waits for the N64DEL2 merge.
- The 20 active site lines (gSP/gDP calls, nd_scomp_rec), in 4 straight-line blocks:
  B1 gfx_build_crystal_ball_pre :80-84 (5); B2 gfx_build_crystal_ball_post :97, :101 (2);
  B3 gfx_build_inside_crystal_ball after the `if (!(z > 0.0f)) return;` at :123: :167-170, :179, :182-185, :213 (10);
  B4 inside the `if (ulx >= 0 && uly >= 0 && ...)` at :203: :204, :207, :208 (3). These numbers are main line numbers. N64DEL2
  moves them: re-read the 4 blocks after its merge (same function and guard names).
- 90 % rule: at least 18 of the 20 sites. B1+B3+B4 = 18 passes. B3+B4 = 13 fails. So the scene must run B1 and B3 and B4.
- The drec switch is `drec=<first>:<count>` (pm_gpu.c:574). bench kind azahar_pic. The drec.txt cmp is as spot_mac01_drec
  (bench.d/HOSTCORE2.json).
Start (after N64DEL2 merges and the user gives the scene): `sh 3ds/tools/wt_new.sh GOLDWLD0`. PM_ROW=GOLDWLD0 PM_LANE=GOLDWLD0.
## Touch only (places)
crystal_ball_gfx.c: 4 lines `ND_COV(ND_COV_CB, k);` (k = 0..3), one at the top of each block B1-B4. New 3ds/include/nd_cov.h
and 3ds/src/nd_cov.c. New azt.d/GOLDWLD0.json and bench.d/GOLDWLD0.json.
## Exact edits
1. nd_cov.h: `enum { ND_COV_CB = 0, ND_COV_NFILE };` `extern int nd_cov_on;` `void nd_cov_hit(unsigned file, unsigned k);`
   `#define ND_COV(f, k) do { if (__builtin_expect(nd_cov_on, 0)) nd_cov_hit((f), (k)); } while (0)`.
2. nd_cov.c: nd_cov_on is set at the first frame from `pm_debug_has("cov")`. nd_cov_hit sets bit k of a mask per file (32 bits).
   At each change of pm_frame_n / 100 it calls `pm_log_evt("cov f%u file %u mask %08x", pm_frame_n, file, mask)` for every file
   with a mask that is not 0.
3. crystal_ball_gfx.c: `#include "nd_cov.h"` after the last include, and the 4 ND_COV lines.
4. Scene (commit before any code is based): azt.d/GOLDWLD0.json `crystal_ball_drec` with the user's scene. Its `debug` field
   adds `det cov drec=<F>:2`, where F is a frame in which the ball is drawn. The until is `drec f<F+1> draws [1-9]\d* bytes`.
   bench.d/GOLDWLD0.json: kind azahar_pic, as spot_mac01_drec.
5. Commit: `GOLDWLD0: coverage marks of crystal_ball_gfx.c (cov switch), crystal_ball_drec golden scene`.
## Gate (expected values)
1. Build, memcheck OK. nd_host PASS.
2. Coverage: the log of `bench.py run crystal_ball_drec --row GOLDWLD0 --why "GOLDWLD0 coverage"` has
   `cov f\d+ file 0 mask 0000000d` or `0000000f` (B1, B3, B4 = 18 sites or all 20). Any other mask: stop and hand back.
   Seed first (a value change): ND_COV(ND_COV_CB, 0) -> ND_COV(ND_COV_CB, 4) in B1. The mask must be 0000001c or 0000001e.
3. Golden base: `bench.py base crystal_ball_drec` (two runs, drec.txt byte-equal). A differing pair is a tool fault.
4. No draw change: `bench.py base spot_mac01_drec` at the merge-base. Then run it and cmp: byte-equal (switch off).
5. Budget 8 Azahar starts. Counter: none moves (phase G golden capture, plan v3 section 6).
## Stop and end
A mask below the rule, a base pair that differs, a drec difference in gate 4, or a tool fault: stop, commit, hand back the exact
output (CLAUDE.md 4-5). Delete this row file in the completing commit.

# Row CODEMODFX3: codemod run on src/effects/explosion.c (10 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 182ac5091)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/explosion.c` on main: `value 10 keep 0 state 0 dl 4`.
- The file has no TARGET_3DS line. Its includes end with `#include "nd_gtab.h"` (line 3). The tool adds no #include.
- Scene: fxpic_20 (fxtest 20 = explosion, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source (correction 1, ORACLE8 2026-11-13): explosion.c:227 on branch CODEMODFX3 (2fd29e760),
  `nd_val_prim(0, 0, 255, 255, primB, primAlpha);`. The old seed at :193 is dead in this scene. fxtest spawns
  `fx_explosion(0, X, Y, Z)` (pm_fxtest.c:52). With type 0, explosion_main sets part 1 `unk_00 = -1` (explosion.c:56-58), so the
  `if (part->unk_00 != -1)` block (:187-196) never runs. :227 always runs. D_09000910_344880 (nd_rec_dl at :201) sets
  `gsDPSetCombineLERP(0, 0, 0, PRIMITIVE, ...)`, so the colour output is the prim RGB, and part 2 starts with alpha 255.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX3`. PM_ROW=CODEMODFX3 PM_LANE=CODEMODFX3 on every call.
## Touch only (places)
src/effects/explosion.c only (the 10 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/explosion.c` (rewrites in place).
2. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
3. Commit: `CODEMODFX3: codemod run on explosion.c (10 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/explosion.c` after the edit: `value 0 keep 0 state 0 dl 4`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of explosion.c falls by 10. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX3 --why "CODEMODFX3"`: PASS.
4. Byte compare: `bench.py base fxpic_20` at the merge-base, then `bench.py run fxpic_20 --row CODEMODFX3 --why "CODEMODFX3 byte gate"` and
   `bench.py cmp fxpic_20 --row CODEMODFX3`: every shot byte-equal.
5. Seeded fault (a value change, then undo): at :227 the third argument 255 -> 128, i.e. `nd_val_prim(0, 0, 128, 255, primB,
   primAlpha);`. Before the run, `sha1sum build3ds/lane-CODEMODFX3/pm_3ds.elf` must differ from its value at the real edit
   (the build number does not change for an uncommitted edit, so it proves nothing). cmp fxpic_20 must show a difference.
   If it shows none: stop and hand back. Budget: one reset for this oracle correction (CLAUDE.md 4).
6. Budget 8 Azahar starts. Counter: C1 -10, C2 -10.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
